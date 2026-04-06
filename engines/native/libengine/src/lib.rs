//! Implements the engine API to support benchmarking native applications.
//! Supports the collection of execution time. Compilation time from high level
//! language to native binary is not recorded since (a) compilation is currently
//! a separate step taken by the user and (b) Wasm also does not record the
//! equivalent step. Instantiation time is also not recorded. Since the Sightglass
//! framework is focused on Wasm first and because this is file is based on the
//! benchmarking supporting for Wasmtime, some function and variable names are
//! consistent with the assumption that we are executing a Wasm file.
//!

use anyhow::{Context, Result};
use nix::libc::fflush;
use nix::unistd::{close, dup, dup2};
use std::env;
use std::os::raw::{c_int, c_void};
use std::os::fd::AsRawFd;
use std::path::{Path, PathBuf};
use std::ptr;
use std::slice;
use std::{
    fs::File,
    io::{self, Write},
};

pub type ExitCode = c_int;
pub const OK: ExitCode = 0;
pub const ERR: ExitCode = -1;

// Configuration options for the benchmark.
#[repr(C)]
pub struct WasmBenchConfig {
    // The working directory where benchmark should be executed.
    pub working_dir_ptr: *const u8,
    pub working_dir_len: usize,

    /// The file path that should be created and used as `stdout`.
    pub stdout_path_ptr: *const u8,
    pub stdout_path_len: usize,

    /// The file path that should be created and used as `stderr`.
    pub stderr_path_ptr: *const u8,
    pub stderr_path_len: usize,

    /// The (optional) file path that should be opened and used as `stdin`. If
    /// not provided, then the WASI context will not have a `stdin` initialized.
    pub stdin_path_ptr: *const u8,
    pub stdin_path_len: usize,

    /// The functions to start and stop performance timers/counters during Wasm
    /// compilation.
    pub compilation_timer: *mut u8,
    pub compilation_start: extern "C" fn(*mut u8),
    pub compilation_end: extern "C" fn(*mut u8),

    /// The functions to start and stop performance timers/counters during Wasm
    /// instantiation.
    pub instantiation_timer: *mut u8,
    pub instantiation_start: extern "C" fn(*mut u8),
    pub instantiation_end: extern "C" fn(*mut u8),

    /// The functions to start and stop performance timers/counters during Wasm
    /// execution.
    pub execution_timer: *mut u8,
    pub execution_start: extern "C" fn(*mut u8),
    pub execution_end: extern "C" fn(*mut u8),

    pub working_dir: PathBuf,
    pub stdout_file: File,
    pub stderr_file: File,
}

impl WasmBenchConfig {
    fn working_dir(&self) -> Result<PathBuf> {
        let working_dir =
            unsafe { std::slice::from_raw_parts(self.working_dir_ptr, self.working_dir_len) };
        let working_dir = std::str::from_utf8(working_dir)
            .context("given working directory is not valid UTF-8")?;
        Ok(working_dir.into())
    }

    fn stdout_path(&self) -> Result<PathBuf> {
        let stdout_path =
            unsafe { std::slice::from_raw_parts(self.stdout_path_ptr, self.stdout_path_len) };
        let stdout_path =
            std::str::from_utf8(stdout_path).context("given stdout path is not valid UTF-8")?;
        Ok(stdout_path.into())
    }

    fn stderr_path(&self) -> Result<PathBuf> {
        let stderr_path =
            unsafe { std::slice::from_raw_parts(self.stderr_path_ptr, self.stderr_path_len) };
        let stderr_path =
            std::str::from_utf8(stderr_path).context("given stderr path is not valid UTF-8")?;
        Ok(stderr_path.into())
    }
}

// Exposes a C-compatible way of creating the engine from the bytes of a single
// Wasm module.
//
// On success, the `out_bench_ptr` is initialized to a pointer to a structure
// that contains the engine's initialized state, and `0` is returned. On
// failure, a non-zero status code is returned and `out_bench_ptr` is left
// untouched.
#[no_mangle]
pub extern "C" fn wasm_bench_create(
    config: WasmBenchConfig,
    out_bench_ptr: *mut *mut c_void,
) -> ExitCode {
    let result = (|| -> Result<_> {
        let working_dir = config.working_dir()?;
        let stdout_path = config.stdout_path()?;
        let stderr_path = config.stderr_path()?;

        let stdout = std::fs::File::create(&stdout_path)
            .with_context(|| format!("failed to create {}", stdout_path.display()))?;
        let stderr = std::fs::File::create(&stderr_path)
            .with_context(|| format!("failed to create {}", stderr_path.display()))?;

        let state = Box::new(BenchState::new(
            config.compilation_timer,
            config.compilation_start,
            config.compilation_end,
            config.instantiation_timer,
            config.instantiation_start,
            config.instantiation_end,
            config.execution_timer,
            config.execution_start,
            config.execution_end,
            working_dir,
            stdout_path,
            stderr_path,
            stdout,
            stderr,
        )?);
        Ok(Box::into_raw(state) as _)
    })();

    if let Ok(bench_ptr) = result {
        unsafe {
            assert!(!out_bench_ptr.is_null());
            *out_bench_ptr = bench_ptr;
        }
    }

    to_exit_code(result.map(|_| ()))
}

// Free the engine state allocated by this library.
#[no_mangle]
pub extern "C" fn wasm_bench_free(state: *mut c_void) {
    assert!(!state.is_null());
    unsafe {
        drop(Box::from_raw(state as *mut BenchState));
    }
}

// Compile the benchmark module.
#[no_mangle]
pub extern "C" fn wasm_bench_compile(
    state: *mut c_void,
    wasm_bytes: *const u8,
    wasm_bytes_length: usize,
) -> ExitCode {
    let state = unsafe { (state as *mut BenchState).as_mut().unwrap() };
    let wasm_bytes = unsafe { slice::from_raw_parts(wasm_bytes, wasm_bytes_length) };
    let result = state.compile(wasm_bytes).context("failed to compile");
    to_exit_code(result)
}

// Instantiate the benchmark module.
#[no_mangle]
pub extern "C" fn wasm_bench_instantiate(state: *mut c_void) -> ExitCode {
    let state = unsafe { (state as *mut BenchState).as_mut().unwrap() };
    let result = state.instantiate().context("failed to instantiate");
    to_exit_code(result)
}

// Execute the benchmark module.
#[no_mangle]
pub extern "C" fn wasm_bench_execute(state: *mut c_void) -> ExitCode {
    let state = unsafe { (state as *mut BenchState).as_mut().unwrap() };
    let result = state.execute().context("failed to execute");
    to_exit_code(result)
}

// Helper function for converting a Rust result to a C error code.
// This will print an error indicating some information regarding the failure.
fn to_exit_code<T>(result: impl Into<Result<T>>) -> ExitCode {
    match result.into() {
        Ok(_) => OK,
        Err(error) => {
            eprintln!("{:?}", error);
            ERR
        }
    }
}

// This structure contains the actual Rust implementation of the
// benchmarking state
struct BenchState {
    compilation_timer: *mut u8,
    compilation_start: extern "C" fn(*mut u8),
    compilation_end: extern "C" fn(*mut u8),
    instantiation_timer: *mut u8,
    instantiation_start: extern "C" fn(*mut u8),
    instantiation_end: extern "C" fn(*mut u8),
    execution_timer: *mut u8,
    execution_start: extern "C" fn(*mut u8),
    execution_end: extern "C" fn(*mut u8),
    working_dir: PathBuf,
    stdout_path: PathBuf,
    stderr_path: PathBuf,
    stdout_file: File,
    stderr_file: File,
}

static mut NATIVE_EXECUTION_TIMER: Option<*mut u8> = None;
static mut NATIVE_EXECUTION_START: Option<extern "C" fn(*mut u8)> = None;
static mut NATIVE_EXECUTION_END: Option<extern "C" fn(*mut u8)> = None;

fn flush_process_stdio() {
    let _ = io::stdout().flush();
    let _ = io::stderr().flush();
    unsafe { fflush(ptr::null_mut()) };
}

struct RedirectedStdio {
    original_stdout: c_int,
    original_stderr: c_int,
}

impl RedirectedStdio {
    fn new(stdout_file: &File, stderr_file: &File) -> Result<Self> {
        flush_process_stdio();

        let original_stdout = dup(1)?;
        let original_stderr = dup(2)?;

        if let Err(err) = dup2(stdout_file.as_raw_fd(), 1) {
            let _ = close(original_stdout);
            let _ = close(original_stderr);
            return Err(err.into());
        }
        if let Err(err) = dup2(stderr_file.as_raw_fd(), 2) {
            let _ = dup2(original_stdout, 1);
            let _ = close(original_stdout);
            let _ = close(original_stderr);
            return Err(err.into());
        }

        Ok(Self {
            original_stdout,
            original_stderr,
        })
    }
}

impl Drop for RedirectedStdio {
    fn drop(&mut self) {
        flush_process_stdio();
        let _ = dup2(self.original_stdout, 1);
        let _ = dup2(self.original_stderr, 2);
        let _ = close(self.original_stdout);
        let _ = close(self.original_stderr);
    }
}

#[no_mangle]
pub extern "C" fn bench_start() {
    let timer = unsafe { NATIVE_EXECUTION_TIMER.unwrap() };
    unsafe { NATIVE_EXECUTION_START.unwrap()(timer) };
}

#[no_mangle]
pub extern "C" fn bench_end() {
    let timer = unsafe { NATIVE_EXECUTION_TIMER.unwrap() };
    unsafe { NATIVE_EXECUTION_END.unwrap()(timer) };
}

impl BenchState {
    fn reopen_output_files(&mut self) -> Result<()> {
        self.stdout_file = File::create(&self.stdout_path)
            .with_context(|| format!("failed to create {}", self.stdout_path.display()))?;
        self.stderr_file = File::create(&self.stderr_path)
            .with_context(|| format!("failed to create {}", self.stderr_path.display()))?;
        Ok(())
    }

    fn new(
        compilation_timer: *mut u8,
        compilation_start: extern "C" fn(*mut u8),
        compilation_end: extern "C" fn(*mut u8),
        instantiation_timer: *mut u8,
        instantiation_start: extern "C" fn(*mut u8),
        instantiation_end: extern "C" fn(*mut u8),
        execution_timer: *mut u8,
        execution_start: extern "C" fn(*mut u8),
        execution_end: extern "C" fn(*mut u8),
        working_dir: PathBuf,
        stdout_path: PathBuf,
        stderr_path: PathBuf,
        stdout_file: File,
        stderr_file: File,
    ) -> Result<Self> {
        Ok(Self {
            compilation_timer,
            compilation_start,
            compilation_end,
            instantiation_timer,
            instantiation_start,
            instantiation_end,
            execution_timer,
            execution_start,
            execution_end,
            working_dir,
            stdout_path,
            stderr_path,
            stdout_file,
            stderr_file,
        })
    }

    fn compile(&mut self, _bytes: &[u8]) -> Result<()> {
        (self.compilation_start)(self.compilation_timer);
        (self.compilation_end)(self.compilation_timer);
        Ok(())
    }

    fn instantiate(&mut self) -> Result<()> {
        (self.instantiation_start)(self.instantiation_timer);
        (self.instantiation_end)(self.instantiation_timer);
        Ok(())
    }

    fn execute(&mut self) -> Result<()> {
        // Initialize call back timers
        unsafe {
            NATIVE_EXECUTION_TIMER = Some(self.execution_timer);
            NATIVE_EXECUTION_START = Some(self.execution_start);
            NATIVE_EXECUTION_END = Some(self.execution_end);
        }

        // Load the library
        let native_lib = self.working_dir.join("./benchmark.so");
        let native_compiled_code_lib = unsafe { libloading::Library::new(&native_lib)? };
        let native_entry: libloading::Symbol<'_, unsafe extern "C" fn() -> i32> =
            unsafe { native_compiled_code_lib.get(b"native_entry").unwrap() };

        // Set working directory for loading any input files.
        let root_dir = env::current_dir()?;
        let working_dir = Path::new(&self.working_dir);
        assert!(env::set_current_dir(&working_dir).is_ok());
        self.reopen_output_files()?;

        {
            let _stdio = RedirectedStdio::new(&self.stdout_file, &self.stderr_file)?;

            // Run the benchmark with stdout/stderr streamed directly to the
            // configured log files so large outputs cannot fill a pipe buffer.
            unsafe { (native_entry)() };
        }

        // Reset working directory
        assert!(env::set_current_dir(&root_dir).is_ok());

        Ok(())
    }
}
