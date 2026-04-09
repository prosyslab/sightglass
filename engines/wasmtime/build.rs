//! Build a Sightglass engine using Wasmtime. Usage:
//!
//! ```
//! rustc build.rs
//! [REPOSITORY=<repo url>] [REVISION=<hash|branch|tag>] ./build [<destination dir>]
//! ```
//!
//! Note that a `hash` must be the full commit hash.
//!
//! Optional environment variables:
//! - `BUILD_PROFILE`: logical build-profile name recorded in `.build-info`.
//! - `CARGO_PACKAGE_FEATURES`: comma-separated `package/feature` entries to
//!   enable for the engine build.
//! - `BUILD_PATCHES`: comma-separated patch ids to apply before the engine
//!   build.

#![deny(missing_docs)]
#![deny(clippy::all)]
#![warn(clippy::pedantic)]
#![allow(clippy::or_fun_call)]

use std::env;
use std::fs;
use std::fs::File;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::SystemTime;

const JITDUMP_PATCH: &str = include_str!("../../../../compilers/patches/jitdump.patch");

#[derive(Clone, Debug, Eq, Ord, PartialEq, PartialOrd)]
struct CargoPackageFeature {
    package: String,
    feature: String,
}

fn main() {
    // The sole CLI argument is the path at which to place the built engine library and metadata.
    let args: Vec<_> = env::args_os().collect();
    let destination_dir = match args.get(1) {
        Some(p) => Path::new(p)
            .canonicalize()
            .expect("the first parameter is not a valid directory"),
        None => env::current_dir().unwrap(),
    };

    // Collect configuration for building the engine library from environment variables.
    // - `REPOSITORY` controls the Wasmtime source to pull
    // - `REVISION` is a valid Git identifier: e.g., branch name, tag name, long or short commit
    //   hash.
    // - `BUILD_DIR` is used internally to control where the library is cloned to and built; if not
    //   present, a temporary directory is created and later removed
    let repository =
        var("REPOSITORY").unwrap_or("https://github.com/bytecodealliance/wasmtime/".into());
    let revision = var("REVISION").unwrap_or("main".into());
    let build_profile = var("BUILD_PROFILE").unwrap_or("bench".into());
    let cargo_package_features =
        parse_package_features(&var("CARGO_PACKAGE_FEATURES").unwrap_or_default());
    let build_patches = parse_patch_ids(&var("BUILD_PATCHES").unwrap_or_default());
    let (build_dir, remove_build_dir) = if let Some(p) = env::var_os("BUILD_DIR") {
        let p = PathBuf::from(p)
            .canonicalize()
            .expect("BUILD_DIR must be a valid directory");
        (p, false)
    } else {
        (create_temp_directory(), true)
    };

    // Clone the repository at the specified revision. The sequence below is more space-efficient
    // (and thus faster) than cloning the entire repository.
    section("Retrieving the repository");
    exec(&["git", "init"], &build_dir);
    exec(&["git", "remote", "add", "origin", &repository], &build_dir);
    exec(
        &["git", "fetch", "--depth", "1", "origin", &revision],
        &build_dir,
    );
    exec(&["git", "checkout", "FETCH_HEAD"], &build_dir);
    exec(
        &["git", "submodule", "update", "--init", "--depth", "1"],
        &build_dir,
    );
    if !build_patches.is_empty() {
        section("Applying local patches");
        apply_named_patches(&build_dir, &build_patches);
    }

    // Build the engine library.
    section("Building the engine");
    let cargo_build_cmd = build_engine_command(&cargo_package_features);
    exec(&cargo_build_cmd, &build_dir);

    // Construct a `.build-info` file that will capture the important details a user would want to
    // know if attempting to replicate benchmark results. (The current set is not exhaustive!).
    section("Collecting metadata");
    let build_info = write_buildinfo(
        &build_dir,
        &repository,
        &revision,
        &build_profile,
        &cargo_package_features,
        &build_patches,
    );
    let build_info_contents =
        fs::read_to_string(&build_info).expect("unable to read .build-info file");
    eprintln!("{}", build_info_contents);

    // Finally, the generated files are copied to their destination and we clean up the build
    // directory.
    section("Copying files to destination");
    let from_engine_library = build_dir
        .join("target/release")
        .join(as_library_filename("wasmtime_bench_api"));
    let to_engine_library = destination_dir.join(as_library_filename("engine"));
    copy(from_engine_library, to_engine_library);
    let to_build_info = destination_dir.join(".build-info");
    copy(build_info, to_build_info);
    if remove_build_dir {
        eprintln!(
            "Removing temporary build directory: {}",
            build_dir.display()
        );
        fs::remove_dir_all(&build_dir).expect("unable to clean up temporary build directory");
    }
}

/// Print a section header for logging.
fn section(title: &str) {
    eprintln!();
    eprintln!("===== {} =====", title);
}

/// Helpful wrapper to access an environment variable as a string. `env::var` returns an error when
/// the `OsString` cannot be converted, which is not exactly what we want. This function panics if the
/// string cannot be converted but still returns an `Option` indicating if the variable was present.
fn var(key: &str) -> Option<String> {
    env::var_os(key).map(|s| {
        s.into_string()
            .expect("the given value could not be converted to UTF-8")
    })
}

/// Helpful wrapper to create a temporary directory; e.g., `/tmp/sightglass-wasmtime-build-<current
/// unix seconds>`)
fn create_temp_directory() -> PathBuf {
    let mut p = env::temp_dir();
    let time = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap();
    p.push(format!(
        "sightglass-wasmtime-build-{}.{}",
        time.as_secs(),
        time.subsec_nanos()
    ));
    fs::create_dir(&p).expect("unable to create temporary build directory");
    eprintln!("Creating temporary build directory: {}", p.display());
    p
}

/// Execute a `command` in the `working_directory`, panicking on failure.
fn exec<P, S>(command: &[S], working_directory: P)
where
    P: AsRef<Path>,
    S: AsRef<str>,
{
    eprintln!(
        "> {}",
        command
            .iter()
            .map(AsRef::as_ref)
            .collect::<Vec<_>>()
            .join(" ")
    );
    let mut cmd = Command::new(command[0].as_ref());
    cmd.args(command[1..].iter().map(AsRef::as_ref));
    cmd.current_dir(working_directory);
    let status = cmd.status().expect("unable to execute command");
    assert!(
        status.success(),
        "command failed: {}",
        command
            .iter()
            .map(AsRef::as_ref)
            .collect::<Vec<_>>()
            .join(" ")
    );
}

/// Execute a `command` in the `working_directory` and return whether it succeeded.
fn exec_ok<P, S>(command: &[S], working_directory: P) -> bool
where
    P: AsRef<Path>,
    S: AsRef<str>,
{
    eprintln!(
        "> {}",
        command
            .iter()
            .map(AsRef::as_ref)
            .collect::<Vec<_>>()
            .join(" ")
    );
    let mut cmd = Command::new(command[0].as_ref());
    cmd.args(command[1..].iter().map(AsRef::as_ref));
    cmd.current_dir(working_directory);
    match cmd.status() {
        Ok(status) => status.success(),
        Err(_) => false,
    }
}

/// Same as `exec` but captures the command output.
fn exec_with_stdout<P, S>(command: &[S], working_directory: P) -> String
where
    P: AsRef<Path>,
    S: AsRef<str>,
{
    eprintln!(
        "> {}",
        command
            .iter()
            .map(AsRef::as_ref)
            .collect::<Vec<_>>()
            .join(" ")
    );
    let mut cmd = Command::new(command[0].as_ref());
    cmd.args(command[1..].iter().map(AsRef::as_ref));
    cmd.current_dir(working_directory);
    let out = cmd.output().expect("unable to execute command");
    assert!(out.status.success());
    std::str::from_utf8(&out.stdout).unwrap().trim().to_string()
}

fn apply_named_patches(build_dir: &Path, patch_ids: &[String]) {
    for patch_id in patch_ids {
        let (patch_filename, patch_contents) = match patch_id.as_str() {
            "jitdump" => (".jitdump.patch", JITDUMP_PATCH),
            _ => panic!("unsupported build patch id: {}", patch_id),
        };
        let patch_path = build_dir.join(patch_filename);
        fs::write(&patch_path, patch_contents).unwrap_or_else(|error| {
            panic!(
                "unable to write patch file {}: {}",
                patch_path.display(),
                error
            )
        });
        let patch_path_str = patch_path
            .to_str()
            .expect("patch path should be valid UTF-8");
        if !exec_ok(&["git", "apply", "--check", patch_path_str], build_dir) {
            panic!("patch {} no longer applies cleanly", patch_id);
        }
        exec(&["git", "apply", patch_path_str], build_dir);
    }
}

fn parse_package_features(raw: &str) -> Vec<CargoPackageFeature> {
    let mut features: Vec<_> = raw
        .split(',')
        .map(str::trim)
        .filter(|entry| !entry.is_empty())
        .map(|entry| {
            let (package, feature) = entry
                .split_once('/')
                .unwrap_or_else(|| panic!("expected package/feature entry, got {}", entry));
            CargoPackageFeature {
                package: package.to_owned(),
                feature: feature.to_owned(),
            }
        })
        .collect();
    features.sort_unstable();
    features.dedup();
    features
}

fn render_package_features(features: &[CargoPackageFeature]) -> String {
    features
        .iter()
        .map(|feature| format!("{}/{}", feature.package, feature.feature))
        .collect::<Vec<_>>()
        .join(",")
}

fn render_feature_names(features: &[CargoPackageFeature]) -> String {
    let mut feature_names: Vec<_> = features
        .iter()
        .map(|feature| feature.feature.as_str())
        .collect();
    feature_names.sort_unstable();
    feature_names.dedup();
    feature_names.join(",")
}

fn parse_patch_ids(raw: &str) -> Vec<String> {
    let mut patches: Vec<_> = raw
        .split(',')
        .map(str::trim)
        .filter(|patch| !patch.is_empty())
        .map(str::to_owned)
        .collect();
    patches.sort_unstable();
    patches.dedup();
    patches
}

fn build_engine_command(features: &[CargoPackageFeature]) -> Vec<String> {
    let mut command = vec![
        "cargo".to_owned(),
        "build".to_owned(),
        "--release".to_owned(),
        "-p".to_owned(),
        "wasmtime-bench-api".to_owned(),
        "-p".to_owned(),
        "cranelift-codegen".to_owned(),
    ];
    let mut packages: Vec<_> = features
        .iter()
        .map(|feature| feature.package.as_str())
        .collect();
    packages.sort_unstable();
    packages.dedup();
    for package in packages {
        if package == "cranelift-codegen" {
            continue;
        }
        command.push("-p".to_owned());
        command.push(package.to_owned());
    }
    let feature_names = render_feature_names(features);
    if !feature_names.is_empty() {
        command.push("--features".to_owned());
        command.push(feature_names);
    }
    command
}

/// Collect system metadata used for building the Wasmtime engine and emit a `.build-info` file
/// containing key-value pairs.
fn write_buildinfo<P>(
    build_dir: P,
    repository: &str,
    revision: &str,
    build_profile: &str,
    cargo_package_features: &[CargoPackageFeature],
    build_patches: &[String],
) -> PathBuf
where
    P: AsRef<Path>,
{
    let build_dir = build_dir.as_ref();
    let commit = exec_with_stdout(&["git", "rev-parse", "HEAD"], &build_dir);
    let datetime = exec_with_stdout(
        &["git", "show", "--no-patch", "--no-notes", "--pretty=%cI"],
        &build_dir,
    );
    let cargo = exec_with_stdout(&["cargo", "--version"], &build_dir);
    let rustc = exec_with_stdout(&["rustc", "--version"], &build_dir);
    let build_info = build_dir.join(".build-info");
    eprintln!("Writing metadata to {}:", build_info.display());
    {
        let mut file = File::create(&build_info).expect("failed to create .build-info file");
        writeln!(file, "NAME=wasmtime").unwrap();
        writeln!(file, "REPOSITORY={}", repository).unwrap();
        writeln!(file, "REVISION={}", revision).unwrap();
        writeln!(file, "BUILD_PROFILE={}", build_profile).unwrap();
        writeln!(
            file,
            "CARGO_PACKAGE_FEATURES={}",
            render_package_features(cargo_package_features)
        )
        .unwrap();
        writeln!(file, "BUILD_PATCHES={}", build_patches.join(",")).unwrap();
        writeln!(file, "_COMMIT={}", commit).unwrap();
        writeln!(file, "_COMMIT_DATETIME={}", datetime).unwrap();
        writeln!(file, "_CARGO={}", cargo).unwrap();
        writeln!(file, "_RUSTC={}", rustc).unwrap();
    }
    build_info
}

/// Helpful wrapper to copy a file.
fn copy<P: AsRef<Path>>(from: P, to: P) {
    let from = from.as_ref();
    let to = to.as_ref();
    eprintln!("Copying: {} -> {}", from.display(), to.display());
    fs::copy(from, to).expect("unable to copy file");
}

/// Calculate the library name for a sightglass library on the target operating system: e.g.,
/// `engine.dll`, `libengine.so`.
#[must_use]
pub fn as_library_filename(name: &str) -> String {
    format!(
        "{}{}{}",
        env::consts::DLL_PREFIX,
        name,
        env::consts::DLL_SUFFIX
    )
}
