#include "flac_wasi_compat.h"

int flac_wasi_chmod(const char *path, mode_t mode) {
  (void)path;
  (void)mode;
  return 0;
}

int flac_wasi_chown(const char *path, uid_t owner, gid_t group) {
  (void)path;
  (void)owner;
  (void)group;
  return 0;
}
