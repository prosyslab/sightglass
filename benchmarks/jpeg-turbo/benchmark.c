#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sightglass.h"
#include "turbojpeg.h"

#define INPUT_PATH "./default.jpg"
#define ITERATIONS 128
#define JPEG_QUALITY 85

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

static void print_tj_error(const char *operation, tjhandle handle) {
  fprintf(stderr, "%s failed: %s\n", operation, tjGetErrorStr2(handle));
}

int main(void) {
  size_t input_size = 0;
  unsigned char *input = read_input(&input_size);
  if (input == NULL) {
    return 1;
  }

  tjhandle decompressor = tjInitDecompress();
  tjhandle compressor = tjInitCompress();
  if (decompressor == NULL || compressor == NULL) {
    fprintf(stderr, "failed to initialize TurboJPEG handles\n");
    tjDestroy(decompressor);
    tjDestroy(compressor);
    free(input);
    return 1;
  }

  int width = 0;
  int height = 0;
  int subsamp = 0;
  int colorspace = 0;
  if (tjDecompressHeader3(decompressor, input, input_size, &width, &height,
                          &subsamp, &colorspace) != 0) {
    print_tj_error("jpeg header decode", decompressor);
    tjDestroy(decompressor);
    tjDestroy(compressor);
    free(input);
    return 1;
  }

  size_t rgb_size = (size_t)width * (size_t)height * 3;
  unsigned char *rgb = (unsigned char *)malloc(rgb_size);
  unsigned char *roundtrip_rgb = (unsigned char *)malloc(rgb_size);
  if (rgb == NULL || roundtrip_rgb == NULL) {
    fprintf(stderr, "failed to allocate RGB buffers\n");
    free(rgb);
    free(roundtrip_rgb);
    tjDestroy(decompressor);
    tjDestroy(compressor);
    free(input);
    return 1;
  }

  unsigned char *encoded = NULL;
  unsigned long encoded_size = 0;
  uint64_t checksum = 1469598103934665603ULL;

  bench_start();
  for (int iteration = 0; iteration < ITERATIONS; iteration++) {
    if (tjDecompress2(decompressor, input, input_size, rgb, width, 0, height,
                      TJPF_RGB, TJFLAG_FASTDCT) != 0) {
      bench_end();
      print_tj_error("jpeg decode", decompressor);
      free(rgb);
      free(roundtrip_rgb);
      tjFree(encoded);
      tjDestroy(decompressor);
      tjDestroy(compressor);
      free(input);
      return 1;
    }

    tjFree(encoded);
    encoded = NULL;
    encoded_size = 0;
    if (tjCompress2(compressor, rgb, width, 0, height, TJPF_RGB, &encoded,
                    &encoded_size, TJSAMP_420, JPEG_QUALITY,
                    TJFLAG_FASTDCT) != 0) {
      bench_end();
      print_tj_error("jpeg encode", compressor);
      free(rgb);
      free(roundtrip_rgb);
      tjFree(encoded);
      tjDestroy(decompressor);
      tjDestroy(compressor);
      free(input);
      return 1;
    }

    if (tjDecompress2(decompressor, encoded, encoded_size, roundtrip_rgb, width,
                      0, height, TJPF_RGB, TJFLAG_FASTDCT) != 0) {
      bench_end();
      print_tj_error("roundtrip jpeg decode", decompressor);
      free(rgb);
      free(roundtrip_rgb);
      tjFree(encoded);
      tjDestroy(decompressor);
      tjDestroy(compressor);
      free(input);
      return 1;
    }

    checksum = fnv1a_update_bytes(checksum, encoded, encoded_size);
    checksum = fnv1a_update_bytes(checksum, roundtrip_rgb, rgb_size);
  }
  bench_end();

  printf("iterations: %d\n", ITERATIONS);
  printf("input_bytes: %zu\n", input_size);
  printf("width: %d\n", width);
  printf("height: %d\n", height);
  printf("input_subsampling: %d\n", subsamp);
  printf("input_colorspace: %d\n", colorspace);
  printf("encoded_bytes: %lu\n", encoded_size);
  printf("jpeg_quality: %d\n", JPEG_QUALITY);
  printf("checksum: %016" PRIx64 "\n", checksum);

  free(rgb);
  free(roundtrip_rgb);
  tjFree(encoded);
  tjDestroy(decompressor);
  tjDestroy(compressor);
  free(input);
  return 0;
}
