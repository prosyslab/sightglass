#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rnnoise.h"
#include "sightglass.h"

#define INPUT_PATH "./default.input"

static uint64_t fnv1a_update_u16(uint64_t hash, uint16_t value) {
  hash ^= value & 0xff;
  hash *= 1099511628211ULL;
  hash ^= value >> 8;
  hash *= 1099511628211ULL;
  return hash;
}

static int16_t clamp_to_i16(float value) {
  long rounded = lrintf(value);
  if (rounded < -32768) {
    return -32768;
  }
  if (rounded > 32767) {
    return 32767;
  }
  return (int16_t)rounded;
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
  if (size < 0) {
    fprintf(stderr, "failed to size %s\n", INPUT_PATH);
    fclose(file);
    return NULL;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "failed to rewind %s\n", INPUT_PATH);
    fclose(file);
    return NULL;
  }

  unsigned char *data = malloc((size_t)size);
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

  int frame_size = rnnoise_get_frame_size();
  if (frame_size != 480) {
    fprintf(stderr, "unexpected RNNoise frame size: %d\n", frame_size);
    free(input);
    return 1;
  }
  size_t frame_bytes = (size_t)frame_size * sizeof(int16_t);
  if (input_size % frame_bytes != 0) {
    fprintf(stderr, "input size %zu is not a multiple of frame size %zu\n",
            input_size, frame_bytes);
    free(input);
    return 1;
  }

  DenoiseState *state = rnnoise_create(NULL);
  if (state == NULL) {
    fprintf(stderr, "failed to create RNNoise state\n");
    free(input);
    return 1;
  }

  float in[480];
  float out[480];
  uint64_t checksum = 1469598103934665603ULL;
  size_t frames = input_size / frame_bytes;

  bench_start();
  for (size_t frame = 0; frame < frames; frame++) {
    const unsigned char *frame_data = input + frame * frame_bytes;
    for (int i = 0; i < frame_size; i++) {
      uint16_t lo = frame_data[i * 2];
      uint16_t hi = frame_data[i * 2 + 1];
      uint16_t raw = (uint16_t)(lo | (hi << 8));
      in[i] = (float)(int16_t)raw;
    }

    rnnoise_process_frame(state, out, in);

    for (int i = 0; i < frame_size; i++) {
      checksum = fnv1a_update_u16(checksum, (uint16_t)clamp_to_i16(out[i]));
    }
  }
  bench_end();

  rnnoise_destroy(state);
  free(input);

  printf("frames: %zu\n", frames);
  printf("checksum: %016llx\n", (unsigned long long)checksum);
  return 0;
}
