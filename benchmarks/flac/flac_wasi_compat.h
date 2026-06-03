#ifndef FLAC_WASI_COMPAT_H
#define FLAC_WASI_COMPAT_H

#include <sys/stat.h>
#include <sys/types.h>

int flac_wasi_chmod(const char *path, mode_t mode);
int flac_wasi_chown(const char *path, uid_t owner, gid_t group);

#endif
