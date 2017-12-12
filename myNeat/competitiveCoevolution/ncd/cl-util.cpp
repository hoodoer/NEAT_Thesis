#ifdef LZMA
#include <cl-util.h>
#include <cl-internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

struct CLSimpleDatum *CLNewSimpleDatumFromCString(const char *str) {
    SETFUNCNAME(CLNewSimpleDatumFromCString);
    ENSURE_TRUE(str != NULL, "string cannot be NULL");
    struct CLSimpleDatum *result = CLNewSimpleDatum(str, strlen(str));
    return result;
}

struct CLDatum *CLNewDatumFromFile(const char *filename) {
    SETFUNCNAME(CLNewDatumFromFile);
    ENSURE_TRUE(filename != NULL, "filename cannot be NULL");
    ENSURE_TRUE(strlen(filename) > 0, "filename cannot be empty");

    struct CLStatT s = CLStat(filename);

    ENSURE_TRUE(s.fExists, "cannot find file: %s ", filename);
    ENSURE_TRUE(s.fFile, "%s is not a file", filename);

    const char *labelStart;

    labelStart = strrchr(filename, '/');
    if (labelStart == NULL) {
        labelStart = filename;
    } else {
    if (labelStart[1] == '\0') {
        ENSURE_TRUE(0, "%s filename cannot end with a slash", filename);
    }
    labelStart += 1;
}

struct CLSimpleDatum *label = CLNewSimpleDatumFromCString(labelStart);
struct CLSimpleDatum *obj;

if (s.size == 0) {
    obj = CLNewSimpleDatum(NULL, 0);
} else {
FILE *fp = fopen(filename, "rb");
ENSURE_TRUE(fp != NULL, "cannot open file");

char *buf = (char*)calloc(s.size, 1);
fread(buf, s.size, 1, fp);
fclose(fp);
obj = CLNewSimpleDatum(buf, s.size);
free(buf);
}
struct CLDatum *result;
result = CLNewDatum(obj, label);
CLFreeSimpleDatum(obj);
CLFreeSimpleDatum(label);
return result;
}




// Added by Drew Kirkpatrick (drew.kirkpatrick@gmail.com)
// to support integration into experiment software.
struct CLDatum *CLNewDatumFromBuffer(const char *buffer, int bufferSize, const char *bufferLabel)
{
    SETFUNCNAME(CLNewDatumFromBuffer);

    struct CLSimpleDatum *label = CLNewSimpleDatumFromCString(bufferLabel);
    struct CLSimpleDatum *obj;

    if (bufferSize == 0)
    {
        obj = CLNewSimpleDatum(NULL, 0);
    }
    else
    {
        obj = CLNewSimpleDatum(buffer, bufferSize);
    }

    struct CLDatum *result;
    result = CLNewDatum(obj, label);

    CLFreeSimpleDatum(obj);
    CLFreeSimpleDatum(label);
    return result;
}





struct CLDatumList *CLNewDatumListFromDatum(const struct CLDatum *datum) {
    struct CLDatumList *dlist = CLNewDatumList(1);
    CLSetDatumAt(dlist, 0, datum);
    return dlist;
}

struct CLDatumList *CLNewDatumListFromDirectory(const char *dirname) {
    SETFUNCNAME(CLNewDatumListFromDirectory);
    ENSURE_TRUE(dirname != NULL, "directory name cannot be NULL");
    ENSURE_TRUE(dirname[0] != '\0', "directory name cannot be empty");
    int count = 0;
    DIR *d = opendir(dirname);
    ENSURE_TRUE(d != NULL, "cannot open directory %s", dirname);
    char *goodname = strdup(dirname);
    while (goodname[strlen(goodname)] == '/' && strlen(goodname) != 1) {
        goodname[strlen(goodname)] = '\0';
    }
    struct dirent *de;
    int maxlen = strlen(dirname) + 16386;
    char *buf = (char*)calloc(1, maxlen);
    while ((de = readdir(d)) != NULL) {
        sprintf(buf, "%s/%s", goodname, de->d_name);
        struct CLStatT st = CLStat(buf);
        if (st.fExists && st.fFile) {
            count += 1;
        }
    }
    ENSURE_TRUE(count != 0, "directory %s does not contain any files", dirname);
    rewinddir(d);
    struct CLDatumList *result = CLNewDatumList(count);
    int i = 0;
    while ((de = readdir(d)) != NULL) {
        sprintf(buf, "%s/%s", goodname, de->d_name);
        struct CLStatT st = CLStat(buf);
        if (st.fExists && st.fFile) {
            struct CLDatum *datum = CLNewDatumFromFile(buf);
            CLSetDatumAt(result, i, datum);
            CLFreeDatum(datum);
            i += 1;
        }
    }
    closedir(d);
    return result;
}

struct CLDatumList *CLNewDatumListFromCLIArgument(const char *arg, int *isDir) {
    struct CLStatT st = CLStat(arg);
    *isDir = 0;
    if (!st.fExists) {
        struct CLSimpleDatum *sdatum = CLNewSimpleDatumFromCString(arg);
        struct CLDatum *datum = CLNewDatum(sdatum, sdatum);
        struct CLDatumList *result = CLNewDatumListFromDatum(datum);
        CLFreeSimpleDatum(sdatum);
        CLFreeDatum(datum);
        return result;
    }
    if (st.fFile) {
        struct CLDatum *datum = CLNewDatumFromFile(arg);
        struct CLDatumList *result = CLNewDatumListFromDatum(datum);
        CLFreeDatum(datum);
        return result;
    }
    *isDir = 1;
    return CLNewDatumListFromDirectory(arg);
}



// Added by Drew Kirkpatrick (drew.kirkpatrick@gmail.com)
// to support integration into experiment software.
struct CLDatumList *CLNewDatumListFromBuffer(const char *buffer, int bufferSize, const char *bufferLabel)
{
    struct CLDatum     *datum  = CLNewDatumFromBuffer(buffer, bufferSize, bufferLabel);
    struct CLDatumList *result = CLNewDatumListFromDatum(datum);
    CLFreeDatum(datum);
    return result;
}
#endif
