#ifndef __CL_INTERNAL_H
#define __CL_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>

#define SETFUNCNAME(funcName) const char *clFuncName = #funcName

#define ENSURE_TRUE(cond, ...)                                \
do {                                                          \
  if (!(cond)) {                                              \
    fprintf(stderr, "Error %s : ", clFuncName);                     \
    fprintf(stderr, __VA_ARGS__);                             \
    fprintf(stderr, ".\n");                                   \
    exit(1);                                                  \
  }                                                           \
} while(0)

#endif

