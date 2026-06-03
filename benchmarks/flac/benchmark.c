#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FLAC/stream_decoder.h"
#include "sightglass.h"

#define INPUT_PATH "./default.flac"
#define DECODE_ITERATIONS 128

typedef struct {
  const FLAC__byte *data;
  size_t size;
  size_t position;
} MemoryInput;

typedef struct {
  uint64_t checksum;
  uint64_t decoded_samples;
  uint64_t frames;
  FLAC__uint64 total_samples;
  unsigned sample_rate;
  unsigned channels;
  unsigned bits_per_sample;
} DecodeStats;

typedef struct {
  MemoryInput input;
  DecodeStats *stats;
} DecoderContext;

static uint64_t fnv1a_update_u8(uint64_t hash, uint8_t value) {
  hash ^= value;
  hash *= 1099511628211ULL;
  return hash;
}

static uint64_t fnv1a_update_u32(uint64_t hash, uint32_t value) {
  for (int i = 0; i < 4; i++) {
    hash = fnv1a_update_u8(hash, (uint8_t)(value >> (i * 8)));
  }
  return hash;
}

static uint64_t fnv1a_update_u64(uint64_t hash, uint64_t value) {
  for (int i = 0; i < 8; i++) {
    hash = fnv1a_update_u8(hash, (uint8_t)(value >> (i * 8)));
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

static FLAC__StreamDecoderReadStatus read_callback(
    const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes,
    void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  MemoryInput *input = &context->input;
  size_t remaining = input->size - input->position;

  if (*bytes == 0) {
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }
  if (remaining == 0) {
    *bytes = 0;
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
  }
  if (*bytes > remaining) {
    *bytes = remaining;
  }

  memcpy(buffer, input->data + input->position, *bytes);
  input->position += *bytes;
  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderSeekStatus seek_callback(
    const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset,
    void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  MemoryInput *input = &context->input;
  if (absolute_byte_offset > input->size) {
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  }
  input->position = (size_t)absolute_byte_offset;
  return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus tell_callback(
    const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset,
    void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  MemoryInput *input = &context->input;
  *absolute_byte_offset = input->position;
  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus length_callback(
    const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length,
    void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  MemoryInput *input = &context->input;
  *stream_length = input->size;
  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder,
                               void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  MemoryInput *input = &context->input;
  return input->position >= input->size;
}

static FLAC__StreamDecoderWriteStatus write_callback(
    const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
    const FLAC__int32 *const buffer[], void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  DecodeStats *stats = context->stats;

  stats->frames++;
  stats->decoded_samples += frame->header.blocksize;
  stats->checksum = fnv1a_update_u32(stats->checksum, frame->header.blocksize);
  stats->checksum = fnv1a_update_u32(stats->checksum, frame->header.channels);
  stats->checksum =
      fnv1a_update_u32(stats->checksum, frame->header.bits_per_sample);

  for (unsigned channel = 0; channel < frame->header.channels; channel++) {
    if (buffer[channel] == NULL) {
      fprintf(stderr, "FLAC decoder returned a null channel buffer\n");
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
  }

  for (unsigned i = 0; i < frame->header.blocksize; i++) {
    for (unsigned channel = 0; channel < frame->header.channels; channel++) {
      stats->checksum = fnv1a_update_u32(
          stats->checksum, (uint32_t)buffer[channel][i]);
    }
  }

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(const FLAC__StreamDecoder *decoder,
                              const FLAC__StreamMetadata *metadata,
                              void *client_data) {
  (void)decoder;
  DecoderContext *context = (DecoderContext *)client_data;
  DecodeStats *stats = context->stats;

  if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
    stats->total_samples = metadata->data.stream_info.total_samples;
    stats->sample_rate = metadata->data.stream_info.sample_rate;
    stats->channels = metadata->data.stream_info.channels;
    stats->bits_per_sample = metadata->data.stream_info.bits_per_sample;
  }
}

static void error_callback(const FLAC__StreamDecoder *decoder,
                           FLAC__StreamDecoderErrorStatus status,
                           void *client_data) {
  (void)decoder;
  (void)client_data;
  fprintf(stderr, "FLAC decoder error: %s\n",
          FLAC__StreamDecoderErrorStatusString[status]);
}

static int decode_once(const unsigned char *input, size_t input_size,
                       DecodeStats *stats) {
  DecoderContext context;
  context.input.data = input;
  context.input.size = input_size;
  context.input.position = 0;
  context.stats = stats;

  FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
  if (decoder == NULL) {
    fprintf(stderr, "failed to allocate FLAC decoder\n");
    return 0;
  }

  FLAC__stream_decoder_set_md5_checking(decoder, true);

  FLAC__StreamDecoderInitStatus init_status = FLAC__stream_decoder_init_stream(
      decoder, read_callback, seek_callback, tell_callback, length_callback,
      eof_callback, write_callback, metadata_callback, error_callback,
      &context);
  if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
    fprintf(stderr, "failed to initialize FLAC decoder: %s\n",
            FLAC__StreamDecoderInitStatusString[init_status]);
    FLAC__stream_decoder_delete(decoder);
    return 0;
  }

  FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
  FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder);
  FLAC__stream_decoder_finish(decoder);
  FLAC__stream_decoder_delete(decoder);

  if (!ok || state != FLAC__STREAM_DECODER_END_OF_STREAM) {
    fprintf(stderr, "FLAC decode failed in state: %s\n",
            FLAC__StreamDecoderStateString[state]);
    return 0;
  }

  return 1;
}

int main(void) {
  size_t input_size = 0;
  unsigned char *input = read_input(&input_size);
  if (input == NULL) {
    return 1;
  }

  DecodeStats stats;
  memset(&stats, 0, sizeof(stats));
  stats.checksum = 1469598103934665603ULL;

  bench_start();
  for (int iteration = 0; iteration < DECODE_ITERATIONS; iteration++) {
    if (!decode_once(input, input_size, &stats)) {
      bench_end();
      free(input);
      return 1;
    }
  }
  bench_end();

  free(input);

  printf("iterations: %d\n", DECODE_ITERATIONS);
  printf("input_bytes: %zu\n", input_size);
  printf("sample_rate: %u\n", stats.sample_rate);
  printf("channels: %u\n", stats.channels);
  printf("bits_per_sample: %u\n", stats.bits_per_sample);
  printf("total_samples: %" PRIu64 "\n", stats.total_samples);
  printf("decoded_samples: %" PRIu64 "\n", stats.decoded_samples);
  printf("frames: %" PRIu64 "\n", stats.frames);
  printf("checksum: %016" PRIx64 "\n", stats.checksum);
  return 0;
}
