#ifndef os_support_h
#define os_support_h

#include <string.h>

#define OPUS_CLEAR(dst, n) (memset((dst), 0, (n) * sizeof(*(dst))))
#define OPUS_COPY(dst, src, n) (memcpy((dst), (src), (n) * sizeof(*(dst))))
#define OPUS_MOVE(dst, src, n) (memmove((dst), (src), (n) * sizeof(*(dst))))

#endif
