#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sightglass.h"
#include "zstd.h"

#define INPUT_PATH "./default.input"
#define ITERATIONS 4
#define COMPRESSION_LEVEL 3

static uint64_t fnv1a_update_u8(uint64_t hash, uint8_t value) {
  hash ^= value;
  hash *= 1099511628211ULL;
  return hash;
}

static uint64_t fnv1a_update_bytes(uint64_t hash, const unsigned char *data,
                                   size_t len) {
  for (size_t i = 0; i < len; i++) {
    hash = fnv1a_update_u8(hash, data[i]);
  }
  return hash;
}

static unsigned char *read_input(size_t *size_out) {
  FILE *file = fopen(INPUT_PATH, "rb");
  if (file == NULL) {
    fprintf(stderr, "failed to open %s: %s\n", INPUT_PATH, strerror(errno));
    return NULL;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "failed to seek %s\n", INPUT_PATH);
    fclose(file);
    return NULL;
  }

  long size = ftell(file);
  if (size <= 0) {
    fprintf(stderr, "failed to size %s\n", INPUT_PATH);
    fclose(file);
    return NULL;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "failed to rewind %s\n", INPUT_PATH);
    fclose(file);
    return NULL;
  }

  unsigned char *data = (unsigned char *)malloc((size_t)size);
  if (data == NULL) {
    fprintf(stderr, "failed to allocate %ld bytes\n", size);
    fclose(file);
    return NULL;
  }

  size_t read = fread(data, 1, (size_t)size, file);
  fclose(file);
  if (read != (size_t)size) {
    fprintf(stderr, "failed to read %s\n", INPUT_PATH);
    free(data);
    return NULL;
  }

  *size_out = (size_t)size;
  return data;
}

int main(void) {
  size_t input_size = 0;
  unsigned char *input = read_input(&input_size);
  if (input == NULL) {
    return 1;
  }

  size_t compressed_capacity = ZSTD_compressBound(input_size);
  unsigned char *compressed = (unsigned char *)malloc(compressed_capacity);
  unsigned char *decompressed = (unsigned char *)malloc(input_size);
  if (compressed == NULL || decompressed == NULL) {
    fprintf(stderr, "failed to allocate zstd work buffers\n");
    free(input);
    free(compressed);
    free(decompressed);
    return 1;
  }

  size_t compressed_size = 0;
  uint64_t checksum = 1469598103934665603ULL;

  bench_start();
  for (int iteration = 0; iteration < ITERATIONS; iteration++) {
    compressed_size =
        ZSTD_compress(compressed, compressed_capacity, input, input_size,
                      COMPRESSION_LEVEL);
    if (ZSTD_isError(compressed_size)) {
      bench_end();
      fprintf(stderr, "zstd compression failed: %s\n",
              ZSTD_getErrorName(compressed_size));
      free(input);
      free(compressed);
      free(decompressed);
      return 1;
    }

    size_t decompressed_size =
        ZSTD_decompress(decompressed, input_size, compressed, compressed_size);
    if (ZSTD_isError(decompressed_size)) {
      bench_end();
      fprintf(stderr, "zstd decompression failed: %s\n",
              ZSTD_getErrorName(decompressed_size));
      free(input);
      free(compressed);
      free(decompressed);
      return 1;
    }
    if (decompressed_size != input_size ||
        memcmp(input, decompressed, input_size) != 0) {
      bench_end();
      fprintf(stderr, "zstd round trip verification failed\n");
      free(input);
      free(compressed);
      free(decompressed);
      return 1;
    }

    checksum = fnv1a_update_bytes(checksum, compressed, compressed_size);
    checksum = fnv1a_update_bytes(checksum, decompressed, decompressed_size);
  }
  bench_end();

  printf("iterations: %d\n", ITERATIONS);
  printf("input_bytes: %zu\n", input_size);
  printf("compressed_bytes: %zu\n", compressed_size);
  printf("compression_level: %d\n", COMPRESSION_LEVEL);
  printf("checksum: %016" PRIx64 "\n", checksum);

  free(input);
  free(compressed);
  free(decompressed);
  return 0;
}
