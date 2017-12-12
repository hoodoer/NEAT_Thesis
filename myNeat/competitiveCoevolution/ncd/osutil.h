#ifndef __OSUTIL_H
#define __OSUTIL_H

#include <sys/types.h>

struct CLStatT {
  int fExists;
  int fDirectory;
  int fFile;
  size_t size;
};

struct CLStatT CLStat(const char *filename);

#endif
