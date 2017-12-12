#ifndef __CLCORE_H
#define __CLCORE_H

#include <sys/types.h>

typedef double CLNumber;
struct CLSimpleDatum;
struct CLDatum;
struct CLDatumList;


struct CLVector;
struct CLMatrix;


struct CLSimpleDatum *CLNewSimpleDatum(const void *data, size_t size);
struct CLSimpleDatum *CLConcatSimpleDatums(const struct CLSimpleDatum *a, const struct CLSimpleDatum *b);
struct CLSimpleDatum *CLNewSimpleDatumFromSimpleDatumOrNull(const struct CLSimpleDatum *datum);
const void *CLGetSimpleDatumBytes(const struct CLSimpleDatum *datum);
size_t CLGetSimpleDatumSize(const struct CLSimpleDatum *datum);
struct CLSimpleDatum *CLNewSimpleDatumFromSimpleDatum(const struct CLSimpleDatum *datum);
void CLFreeSimpleDatum(struct CLSimpleDatum *);

struct CLDatum *CLNewDatum(const struct CLSimpleDatum *obj,
                           const struct CLSimpleDatum *label);
struct CLDatum *CLNewDatumFromDatum(const struct CLDatum *datum);
const struct CLSimpleDatum *CLDatumGetObject(const struct CLDatum *datum);
const struct CLSimpleDatum *CLDatumGetLabel(const struct CLDatum *datum);
void CLFreeDatum(struct CLDatum *datum);

struct CLDatumList *CLNewDatumList(size_t size);
size_t CLDatumListSize(const struct CLDatumList *datumList);
void CLSetDatumAt(struct CLDatumList *datumList, size_t index, const struct CLDatum *datum);
const struct CLDatum *CLGetDatumAt(const struct CLDatumList *datumList, size_t index);
void CLFreeDatumList(struct CLDatumList *datumList);

struct CLVector *CLNewVector(size_t entries);
struct CLVector *CLNewVectorFromVector(const struct CLVector *vec);
void CLSetVectorValue(struct CLVector *vec, size_t index, CLNumber value);
CLNumber CLGetVectorValue(const struct CLVector *vec, size_t index);
void CLFreeVector(struct CLVector *vec);

struct CLMatrix *CLNewMatrix(size_t width, size_t height);
struct CLMatrix *CLNewMatrixFromMatrix(const struct CLMatrix *mat);
void CLSetMatrixValue(struct CLMatrix *mat, size_t x, size_t y, CLNumber value);
CLNumber CLGetMatrixValue(const struct CLMatrix *mat, size_t x, size_t y);
void CLFreeMatrix(struct CLMatrix *mat);

struct CLCompLearnResult *CLNewResultNumber(CLNumber value);
struct CLCompLearnResult *CLNewResultVector(const struct CLVector *vec,
  const struct CLDatumList *lab1);
struct CLCompLearnResult *CLNewResultMatrix(const struct CLMatrix *mat,
  const struct CLDatumList *lab1, const struct CLDatumList *lab2);

int CLIsResultNumber(const struct CLCompLearnResult *result);
CLNumber CLGetNumberResult(const struct CLCompLearnResult *result);

int CLIsResultVector(const struct CLCompLearnResult *result);
const struct CLVector *CLVectorFromResult(const struct CLCompLearnResult *result);

int CLIsResultMatrix(const struct CLCompLearnResult *result);
const struct CLMatrix *CLMatrixFromResult(const struct CLCompLearnResult *result);

void CLFreeResult(struct CLCompLearnResult *result);

CLNumber CLNCDBasic(CLNumber ca, CLNumber cb, CLNumber cab);

#endif
