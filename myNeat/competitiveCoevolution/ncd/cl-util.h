#ifndef __CLUTIL_H
#define __CLUTIL_H

#include <complearn/cl-core.h>
#include <osutil.h>

struct CLSimpleDatum *CLNewSimpleDatumFromCString(const char *str);
struct CLDatum       *CLNewDatumFromFile(const char *filename);
struct CLDatum       *CLNewDatumFromBuffer(const char *buffer, int bufferSize, const char *label);
struct CLDatumList   *CLNewDatumListFromDirectory(const char *dirname);
struct CLDatumList   *CLNewDatumListFromDatum(const struct CLDatum *datum);
struct CLDatumList   *CLNewDatumListFromCLIArgument(const char *arg, int *isDir);
struct CLDatumList   *CLNewDatumListFromBuffer(const char *buffer, int bufferSize, const char *bufferLabel);

#endif
