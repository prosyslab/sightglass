#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fftw3.h"
#include "sightglass.h"

#define INPUT_PATH "./default.input"

typedef struct {
  int iterations;
  uint64_t seed;
  int size_1d;
  int size_2d_x;
  int size_2d_y;
} Workload;

static uint64_t fnv1a_update_u8(uint64_t hash, uint8_t value) {
  hash ^= value;
  hash *= 1099511628211ULL;
  return hash;
}

static uint64_t fnv1a_update_u64(uint64_t hash, uint64_t value) {
  for (int i = 0; i < 8; i++) {
    hash = fnv1a_update_u8(hash, (uint8_t)(value >> (i * 8)));
  }
  return hash;
}

static uint64_t next_random(uint64_t *state) {
  *state = *state * 6364136223846793005ULL + 1442695040888963407ULL;
  return *state;
}

static double random_unit(uint64_t *state) {
  uint64_t value = next_random(state) >> 11;
  double unit = (double)value * (1.0 / 9007199254740992.0);
  return unit * 2.0 - 1.0;
}

static void fill_complex(fftw_complex *data, size_t len, uint64_t *seed) {
  for (size_t i = 0; i < len; i++) {
    data[i][0] = random_unit(seed);
    data[i][1] = random_unit(seed);
  }
}

static uint64_t hash_double(uint64_t hash, double value) {
  uint64_t bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  return fnv1a_update_u64(hash, bits);
}

static uint64_t hash_complex(uint64_t hash, const fftw_complex *data,
                             size_t len) {
  for (size_t i = 0; i < len; i++) {
    hash = hash_double(hash, data[i][0]);
    hash = hash_double(hash, data[i][1]);
  }
  return hash;
}

static int parse_line(const char *line, Workload *workload) {
  char key[64];
  char value[64];
  if (sscanf(line, " %63[^=]=%63s", key, value) != 2) {
    return 1;
  }

  if (strcmp(key, "iterations") == 0) {
    workload->iterations = atoi(value);
  } else if (strcmp(key, "seed") == 0) {
    workload->seed = strtoull(value, NULL, 10);
  } else if (strcmp(key, "size_1d") == 0) {
    workload->size_1d = atoi(value);
  } else if (strcmp(key, "size_2d_x") == 0) {
    workload->size_2d_x = atoi(value);
  } else if (strcmp(key, "size_2d_y") == 0) {
    workload->size_2d_y = atoi(value);
  } else {
    fprintf(stderr, "unknown workload key: %s\n", key);
    return 0;
  }

  return 1;
}

static int read_workload(Workload *workload) {
  memset(workload, 0, sizeof(*workload));

  FILE *file = fopen(INPUT_PATH, "r");
  if (file == NULL) {
    fprintf(stderr, "failed to open %s: %s\n", INPUT_PATH, strerror(errno));
    return 0;
  }

  char line[256];
  while (fgets(line, sizeof(line), file) != NULL) {
    if (line[0] == '\n' || line[0] == '#') {
      continue;
    }
    if (!parse_line(line, workload)) {
      fclose(file);
      return 0;
    }
  }

  if (ferror(file)) {
    fprintf(stderr, "failed to read %s\n", INPUT_PATH);
    fclose(file);
    return 0;
  }
  fclose(file);

  if (workload->iterations <= 0 || workload->size_1d <= 0 ||
      workload->size_2d_x <= 0 || workload->size_2d_y <= 0) {
    fprintf(stderr, "invalid FFTW workload configuration\n");
    return 0;
  }

  return 1;
}

static void *checked_fftw_malloc(size_t bytes, const char *name) {
  void *ptr = fftw_malloc(bytes);
  if (ptr == NULL) {
    fprintf(stderr, "failed to allocate %s (%zu bytes)\n", name, bytes);
  }
  return ptr;
}

int main(void) {
  Workload workload;
  if (!read_workload(&workload)) {
    return 1;
  }

  size_t len_1d = (size_t)workload.size_1d;
  size_t len_2d = (size_t)workload.size_2d_x * (size_t)workload.size_2d_y;

  fftw_complex *in_1d =
      checked_fftw_malloc(sizeof(fftw_complex) * len_1d, "1D input");
  fftw_complex *out_1d =
      checked_fftw_malloc(sizeof(fftw_complex) * len_1d, "1D output");
  fftw_complex *in_2d =
      checked_fftw_malloc(sizeof(fftw_complex) * len_2d, "2D input");
  fftw_complex *out_2d =
      checked_fftw_malloc(sizeof(fftw_complex) * len_2d, "2D output");

  if (in_1d == NULL || out_1d == NULL || in_2d == NULL || out_2d == NULL) {
    fftw_free(in_1d);
    fftw_free(out_1d);
    fftw_free(in_2d);
    fftw_free(out_2d);
    return 1;
  }

  uint64_t planning_seed = workload.seed;
  fill_complex(in_1d, len_1d, &planning_seed);
  fill_complex(in_2d, len_2d, &planning_seed);

  fftw_plan plan_1d =
      fftw_plan_dft_1d(workload.size_1d, in_1d, out_1d, FFTW_FORWARD,
                       FFTW_MEASURE);
  fftw_plan plan_2d = fftw_plan_dft_2d(workload.size_2d_y, workload.size_2d_x,
                                       in_2d, out_2d, FFTW_FORWARD,
                                       FFTW_MEASURE);
  if (plan_1d == NULL || plan_2d == NULL) {
    fprintf(stderr, "failed to create FFTW plans\n");
    if (plan_1d != NULL) {
      fftw_destroy_plan(plan_1d);
    }
    if (plan_2d != NULL) {
      fftw_destroy_plan(plan_2d);
    }
    fftw_free(in_1d);
    fftw_free(out_1d);
    fftw_free(in_2d);
    fftw_free(out_2d);
    return 1;
  }

  uint64_t seed = workload.seed;
  fill_complex(in_1d, len_1d, &seed);
  fill_complex(in_2d, len_2d, &seed);

  bench_start();
  for (int iteration = 0; iteration < workload.iterations; iteration++) {
    fftw_execute(plan_1d);
    fftw_execute(plan_2d);
  }
  bench_end();

  uint64_t checksum = 1469598103934665603ULL;
  checksum = fnv1a_update_u64(checksum, (uint64_t)workload.iterations);
  checksum = fnv1a_update_u64(checksum, (uint64_t)workload.size_1d);
  checksum = fnv1a_update_u64(checksum, (uint64_t)workload.size_2d_x);
  checksum = fnv1a_update_u64(checksum, (uint64_t)workload.size_2d_y);
  checksum = hash_complex(checksum, out_1d, len_1d);
  checksum = hash_complex(checksum, out_2d, len_2d);

  printf("iterations: %d\n", workload.iterations);
  printf("size_1d: %d\n", workload.size_1d);
  printf("size_2d: %dx%d\n", workload.size_2d_x, workload.size_2d_y);
  printf("checksum: %016" PRIx64 "\n", checksum);

  fftw_destroy_plan(plan_1d);
  fftw_destroy_plan(plan_2d);
  fftw_free(in_1d);
  fftw_free(out_1d);
  fftw_free(in_2d);
  fftw_free(out_2d);
  fftw_cleanup();

  return 0;
}
