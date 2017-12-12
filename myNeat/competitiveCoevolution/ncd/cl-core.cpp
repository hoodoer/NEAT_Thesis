#ifdef LZMA
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <complearn/cl-core.h>
#include <cl-internal.h>




struct CLSimpleDatum {
  void *data;
  size_t size; // in bytes
};

struct CLDatum {
  struct CLSimpleDatum *obj;
  struct CLSimpleDatum *label;
};

struct CLDatumList {
  struct CLDatum **dataList;
  size_t size;
};

struct CLVector {
  CLNumber *val;
  int size; // number of entries
};

struct CLMatrix {
  CLNumber *val; // row-major order
  int width, height; // number of columns
};

struct CLSimpleDatum *CLNewSimpleDatum(const void *data, size_t size) {
  SETFUNCNAME(CLNewSimpleDatum);
  struct CLSimpleDatum *result = (CLSimpleDatum*)calloc(sizeof(struct CLSimpleDatum), 1);
  ENSURE_TRUE(size >= 0, "size must be non-negative");
  ENSURE_TRUE(size == 0 || data != NULL, "non-empty data cannot be NULL");
  size_t asize = size;
  if (asize == 0) {
    asize = 1;
  };
  result->data = malloc(asize);
  result->size = size;
  if (size > 0) {
    memcpy(result->data, data, size);
  }
  return result;
}

const void *CLGetSimpleDatumBytes(const struct CLSimpleDatum *datum) {
  SETFUNCNAME(CLGetSimpleDatumBytes);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  return datum->data;
}

size_t CLGetSimpleDatumSize(const struct CLSimpleDatum *datum) {
  SETFUNCNAME(CLGetSimpleDatumSize);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  return datum->size;
}

struct CLSimpleDatum *CLNewSimpleDatumFromSimpleDatum(const struct CLSimpleDatum *datum) {
  SETFUNCNAME(CLNewSimpleDatumFromSimpleDatum);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  return CLNewSimpleDatum(datum->data, datum->size);
}

void CLFreeSimpleDatum(struct CLSimpleDatum *datum) {
  SETFUNCNAME(CLFreeSimpleDatum);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  if (datum->data != NULL) {
    free(datum->data);
  }
  free(datum);
}

struct CLSimpleDatum *CLNewSimpleDatumFromSimpleDatumOrNull(const struct CLSimpleDatum *datum) {
  if (datum == NULL) {
    return NULL;
  }
  return CLNewSimpleDatumFromSimpleDatum(datum);
}

struct CLDatum *CLNewDatum(const struct CLSimpleDatum *obj,
                           const struct CLSimpleDatum *label) {
  struct CLDatum *datum = (CLDatum*)calloc(sizeof(struct CLDatum), 1);
  datum->obj = CLNewSimpleDatumFromSimpleDatum(obj);
  datum->label = CLNewSimpleDatumFromSimpleDatumOrNull(label);
  return datum;
}

struct CLDatum *CLNewDatumFromDatum(const struct CLDatum *datum) {
  SETFUNCNAME(CLNewDatumFromDatum);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  return CLNewDatum(datum->obj, datum->label);
}

const struct CLSimpleDatum *CLDatumGetObject(const struct CLDatum *datum) {
  SETFUNCNAME(CLDatumGetObject);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  return datum->obj;
}

const struct CLSimpleDatum *CLDatumGetLabel(const struct CLDatum *datum) {
  SETFUNCNAME(CLDatumGetLabel);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  return datum->label;
}

void CLFreeDatum(struct CLDatum *datum) {
  SETFUNCNAME(CLFreeDatum);
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  ENSURE_TRUE(datum->obj != NULL, "datum->obj cannot be NULL");
  free(datum->obj);
  free(datum);
}

struct CLDatumList *CLNewDatumList(size_t size) {
  SETFUNCNAME(CLNewDatumList);
  ENSURE_TRUE(size > 0, "datum list size must be greater than 0");
  struct CLDatumList *dl = (CLDatumList*)calloc(sizeof(struct CLDatumList), 1);
  dl->size = size;
  dl->dataList = (CLDatum**)calloc(sizeof(struct CLDatumList *), size);
  return dl;
}

size_t CLDatumListSize(const struct CLDatumList *datumList) {
  SETFUNCNAME(CLDatumListSize);
  ENSURE_TRUE(datumList != NULL, "datum list cannot be NULL");
  return datumList->size;
}

void CLSetDatumAt(struct CLDatumList *datumList, size_t index, const struct CLDatum *datum) {
  SETFUNCNAME(CLSetDatumAt);
  ENSURE_TRUE(datumList != NULL, "datum list cannot be NULL");
  ENSURE_TRUE(datum != NULL, "datum cannot be NULL");
  ENSURE_TRUE(index >= 0, "datum list index cannot be negative");
  ENSURE_TRUE(index < datumList->size, "index must be less than size");
  if (datumList->dataList[index] != NULL) {
    CLFreeDatum(datumList->dataList[index]);
  }
  datumList->dataList[index] = CLNewDatumFromDatum(datum);
}

const struct CLDatum *CLGetDatumAt(const struct CLDatumList *datumList, size_t index) {
  SETFUNCNAME(CLGetDatumAt);
  ENSURE_TRUE(datumList != NULL, "datum list cannot be NULL");
  ENSURE_TRUE(index >= 0, "datum list index cannot be negative");
  ENSURE_TRUE(index < datumList->size, "index must be less than size");
  return datumList->dataList[index];
}

void CLFreeDatumList(struct CLDatumList *datumList) {
  SETFUNCNAME(CLFreeDatumList);
  ENSURE_TRUE(datumList != NULL, "datum list cannot be NULL");
  ENSURE_TRUE(datumList->dataList != NULL, "datumlist datalist cannot be NULL");
  int i;
  for (i = 0; i < datumList->size; ++i) {
    struct CLDatum *datum = datumList->dataList[i];
    if (datum != NULL) {
      CLFreeDatum(datum);
    }
  }
  free(datumList->dataList);
  free(datumList);
}

struct CLVector *CLNewVector(size_t entries) {
  SETFUNCNAME(CLNewVector);
  ENSURE_TRUE(entries > 0, "vector size must be greater than 0");
  struct CLVector *vec = (CLVector*)calloc(sizeof(struct CLVector), 1);
  vec->size = entries;
  vec->val = (CLNumber*)calloc(sizeof(CLNumber), entries);
  int i;
  for (i = 0; i < entries; ++i) {
    vec->val[i] = 0;
  }
  return vec;
}

struct CLVector *CLNewVectorFromVector(const struct CLVector *vec) {
  SETFUNCNAME(CLNewVectorFromVector);
  ENSURE_TRUE(vec != NULL, "vector cannot be NULL");
  struct CLVector *result = (CLVector*)calloc(sizeof(struct CLVector), 1);
  result->size = vec->size;
  result->val = (CLNumber*)calloc(sizeof(CLNumber), vec->size);
  memcpy(result->val, vec->val, sizeof(CLNumber) * vec->size);
  return result;
}

void CLSetVectorValue(struct CLVector *vec, size_t index, CLNumber value) {
  SETFUNCNAME(CLSetVectorValue);
  ENSURE_TRUE(vec != NULL, "vector cannot be NULL");
  ENSURE_TRUE(vec->val != NULL, "vector values cannot be NULL");
  ENSURE_TRUE(index >= 0, "index cannot be negative");
  ENSURE_TRUE(index < vec->size, "index must be less than size");
  vec->val[index] = value;
}

CLNumber CLGetVectorValue(const struct CLVector *vec, size_t index) {
  SETFUNCNAME(CLGetVectorValue);
  ENSURE_TRUE(vec != NULL, "vector cannot be NULL");
  ENSURE_TRUE(vec->val != NULL, "vector values cannot be NULL");
  ENSURE_TRUE(index >= 0, "index cannot be negative");
  ENSURE_TRUE(index < vec->size, "index must be less than size");
  return vec->val[index];
}

void CLFreeVector(struct CLVector *vec) {
  SETFUNCNAME(CLFreeVector);
  ENSURE_TRUE(vec != NULL, "vector cannot be NULL");
  ENSURE_TRUE(vec->val != NULL, "vector values cannot be NULL");
  free(vec->val);
  free(vec);
}

struct CLMatrix *CLNewMatrix(size_t width, size_t height) {
  SETFUNCNAME(CLNewMatrix);
  ENSURE_TRUE(width > 0, "matrix width must be greater than 0");
  ENSURE_TRUE(height > 0, "matrix height must be greater than 0");
  struct CLMatrix *mat = (CLMatrix*)calloc(sizeof(struct CLMatrix), 1);
  mat->width = width;
  mat->height = height;
  mat->val = (CLNumber*)calloc(sizeof(CLNumber), width * height);
  int i;
  for (i = 0; i < width * height; ++i) {
    mat->val[i] = 0;
  }
  return mat;
}

struct CLMatrix *CLNewMatrixFromMatrix(const struct CLMatrix *mat) {
  SETFUNCNAME(CLNewMatrixFromMatrix);
  ENSURE_TRUE(mat != NULL, "matrix cannot be NULL");
  struct CLMatrix *result = (CLMatrix*)calloc(sizeof(struct CLMatrix), 1);
  result->width = mat->width;
  result->height = mat->height;
  result->val = (CLNumber*)calloc(sizeof(CLNumber), mat->width * mat->height);
  memcpy(result->val, mat->val, sizeof(CLNumber) * mat->width * mat->height);
  return result;
}

void CLSetMatrixValue(struct CLMatrix *mat, size_t x, size_t y, CLNumber value){
  SETFUNCNAME(CLSetMatrixValue);
  ENSURE_TRUE(mat != NULL, "matrix cannot be NULL");
  ENSURE_TRUE(x >= 0, "column index cannot be negative");
  ENSURE_TRUE(y >= 0, "row index cannot be negative");
  ENSURE_TRUE(x < mat->width, "column index must be less than width");
  ENSURE_TRUE(y < mat->height, "row index must be less than height");
  ENSURE_TRUE(mat->val != NULL, "matrix values cannot be NULL");
  mat->val[y*mat->width+x] = value;
}

CLNumber CLGetMatrixValue(const struct CLMatrix *mat, size_t x, size_t y) {
  SETFUNCNAME(CLGetMatrixValue);
  ENSURE_TRUE(mat != NULL, "matrix cannot be NULL");
  ENSURE_TRUE(x >= 0, "column index cannot be negative");
  ENSURE_TRUE(y >= 0, "row index cannot be negative");
  ENSURE_TRUE(x < mat->width, "column index must be less than width");
  ENSURE_TRUE(y < mat->height, "row index must be less than height");
  ENSURE_TRUE(mat->val != NULL, "matrix values cannot be NULL");
  return mat->val[y*mat->width+x];
}

void CLFreeMatrix(struct CLMatrix *mat) {
  SETFUNCNAME(CLFreeMatrix);
  ENSURE_TRUE(mat != NULL, "matrix cannot be NULL");
  ENSURE_TRUE(mat->val != NULL, "matrix values cannot be NULL");
  free(mat->val);
  free(mat);
}

struct CLSimpleDatum *CLConcatSimpleDatums(const struct CLSimpleDatum *a, const struct CLSimpleDatum *b) {
  SETFUNCNAME(CLConcatSimpleDatums);
  struct CLSimpleDatum *result = (CLSimpleDatum*)calloc(sizeof(struct CLSimpleDatum), 1);
  ENSURE_TRUE(a != NULL, "first datum cannot be NULL");
  ENSURE_TRUE(b != NULL, "second datum cannot be NULL");
  size_t sa = a->size, sb = b->size;
  result->size = sa + sb;
  if (result->size > 0) {
    result->data = malloc(result->size);
  }
  else {
    result->data = malloc(1);
  }
  if (sa > 0) {
    memcpy(result->data, a->data, sa);
  }
  if (sb > 0) {
    memcpy((char *) (result->data) + sa, b->data, sb);
  }
  return result;
}

CLNumber CLNCDBasic(CLNumber ca, CLNumber cb, CLNumber cab) {
  CLNumber ma = (ca > cb) ? ca : cb;
  CLNumber mi = (ca > cb) ? cb : ca;
  return (cab - mi) / ma;
}
#endif
