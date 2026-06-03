#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>

#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"
#include "draco/mesh/mesh.h"
#include "sightglass.h"

#define INPUT_PATH "./default.drc"
#define DECODE_ITERATIONS 128

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

static uint64_t hash_attribute(uint64_t hash,
                               const draco::PointAttribute *attribute,
                               draco::PointIndex::ValueType num_points) {
  hash = fnv1a_update_u32(hash, (uint32_t)attribute->attribute_type());
  hash = fnv1a_update_u32(hash, (uint32_t)attribute->data_type());
  hash = fnv1a_update_u32(hash, (uint32_t)attribute->num_components());
  hash = fnv1a_update_u64(hash, (uint64_t)attribute->byte_stride());
  hash = fnv1a_update_u64(hash, (uint64_t)attribute->size());
  hash = fnv1a_update_u32(hash, attribute->is_mapping_identity() ? 1 : 0);

  const int64_t byte_stride = attribute->byte_stride();
  if (byte_stride <= 0 || byte_stride > 256) {
    fprintf(stderr, "unexpected Draco attribute byte stride: %lld\n",
            (long long)byte_stride);
    return 0;
  }

  uint8_t value[256];
  for (draco::PointIndex::ValueType point = 0; point < num_points; point++) {
    draco::AttributeValueIndex index =
        attribute->mapped_index(draco::PointIndex(point));
    hash = fnv1a_update_u32(hash, (uint32_t)index.value());

    attribute->GetValue(index, value);
    for (int64_t byte = 0; byte < byte_stride; byte++) {
      hash = fnv1a_update_u8(hash, value[byte]);
    }
  }

  return hash;
}

static uint64_t hash_mesh(const draco::Mesh &mesh) {
  uint64_t hash = 1469598103934665603ULL;
  hash = fnv1a_update_u32(hash, (uint32_t)mesh.num_points());
  hash = fnv1a_update_u32(hash, (uint32_t)mesh.num_faces());
  hash = fnv1a_update_u32(hash, (uint32_t)mesh.num_attributes());

  for (draco::FaceIndex::ValueType face = 0; face < mesh.num_faces(); face++) {
    const draco::Mesh::Face &indices = mesh.face(draco::FaceIndex(face));
    for (int i = 0; i < 3; i++) {
      hash = fnv1a_update_u32(hash, (uint32_t)indices[i].value());
    }
  }

  for (int32_t attr = 0; attr < mesh.num_attributes(); attr++) {
    hash = hash_attribute(hash, mesh.attribute(attr), mesh.num_points());
  }

  return hash;
}

static std::unique_ptr<draco::Mesh> decode_mesh(const unsigned char *input,
                                                size_t input_size) {
  draco::DecoderBuffer buffer;
  buffer.Init(reinterpret_cast<const char *>(input), input_size);

  auto type_status = draco::Decoder::GetEncodedGeometryType(&buffer);
  if (!type_status.ok()) {
    fprintf(stderr, "failed to detect geometry type: %s\n",
            type_status.status().error_msg());
    return nullptr;
  }
  if (type_status.value() != draco::TRIANGULAR_MESH) {
    fprintf(stderr, "expected triangular mesh input\n");
    return nullptr;
  }

  draco::Decoder decoder;
  auto statusor = decoder.DecodeMeshFromBuffer(&buffer);
  if (!statusor.ok()) {
    fprintf(stderr, "failed to decode mesh: %s\n", statusor.status().error_msg());
    return nullptr;
  }

  return std::move(statusor).value();
}

int main(void) {
  size_t input_size = 0;
  unsigned char *input = read_input(&input_size);
  if (input == NULL) {
    return 1;
  }

  uint64_t checksum = 0;
  draco::PointIndex::ValueType points = 0;
  draco::FaceIndex::ValueType faces = 0;
  int32_t attributes = 0;

  bench_start();
  for (int iteration = 0; iteration < DECODE_ITERATIONS; iteration++) {
    std::unique_ptr<draco::Mesh> mesh = decode_mesh(input, input_size);
    if (!mesh) {
      bench_end();
      free(input);
      return 1;
    }

    uint64_t mesh_hash = hash_mesh(*mesh);
    checksum = fnv1a_update_u64(checksum, mesh_hash);
    points = mesh->num_points();
    faces = mesh->num_faces();
    attributes = mesh->num_attributes();
  }
  bench_end();

  free(input);

  printf("iterations: %d\n", DECODE_ITERATIONS);
  printf("input_bytes: %zu\n", input_size);
  printf("points: %u\n", points);
  printf("faces: %u\n", faces);
  printf("attributes: %d\n", attributes);
  printf("checksum: %016llx\n", (unsigned long long)checksum);
  return 0;
}
