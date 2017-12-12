#ifdef LZMA
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cl-internal.h>
#include <osutil.h>

struct CLStatT CLStat(const char *filename) {
  SETFUNCNAME(CLStat);
  ENSURE_TRUE(filename != NULL, "filename cannot be null");

  struct stat st;
  struct CLStatT result;
  memset(&result, 0, sizeof(result));

  int retval = stat(filename, &st);
  if (retval != 0) {
    result.fExists = 0;
    return result;
  }

  result.fExists = 1;
  result.fFile = S_ISREG(st.st_mode);
  result.fDirectory = S_ISDIR(st.st_mode);
  result.size = st.st_size;
  return result;
}
#endif
