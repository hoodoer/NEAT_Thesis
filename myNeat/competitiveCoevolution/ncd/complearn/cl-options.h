#ifndef __CLOPTIONS_H
#define __CLOPTIONS_H

#include <complearn/cl-core.h>

enum CLInputMode {
  CLDirectory, CLFilename, CLLiteral
};

struct CLCompressorOptions {
  int deltaBytes;
};

struct CLCommandOptions {
  struct CLCompressorOptions compression;
};

extern const struct CLCommandOptions CLCommandDefaultOptions;

#endif

