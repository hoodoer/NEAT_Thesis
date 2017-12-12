#ifndef __COMPRESSION_BRIDGE_H
#define __COMPRESSION_BRIDGE_H

#include <sys/types.h>
#include <complearn/cl-core.h>

CLNumber compressed_size_in_bits_and_delta(const char *buf, size_t lenBytes, int deltaByteLen);

#endif
