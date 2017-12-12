#ifdef LZMA
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <complearn/cl-core.h>
#include <complearn/cl-command.h>
#include <cl-internal.h>
#include <complearn/cl-options.h>

#include <compression-bridge.h>

const struct CLCommandOptions CLCommandDefaultOptions = { { 0 } };

struct CLCompLearnCommand {
  const struct CLCompLearnOptions *options;
  int printHelp;
};

struct CLCompLearnResult {
  CLNumber number;

  struct CLVector *vector;

  struct CLMatrix *matrix;
  struct CLDatumList *lab1, *lab2;

};

static struct CLDatumList *CLNewDatumListFromLabels(const struct CLDatumList *dl);

struct CLCompLearnResult *CLNewResultNumber(CLNumber value) {
  struct CLCompLearnResult *result = (CLCompLearnResult*)calloc(sizeof(struct CLCompLearnResult),1);
  result->number = value;
  return result;
}

struct CLCompLearnResult *CLNewResultVector(const struct CLVector *vec,
  const struct CLDatumList *lab1) {
  SETFUNCNAME(CLNewResultVector);
  ENSURE_TRUE(vec != NULL, "vector must not be NULL");
  struct CLCompLearnResult *result = (CLCompLearnResult*)calloc(sizeof(struct CLCompLearnResult),1);
  result->vector = CLNewVectorFromVector(vec);
  if (lab1 != NULL) {
    result->lab1 = CLNewDatumListFromLabels(lab1);
  }
  return result;
}

struct CLCompLearnResult *CLNewResultMatrix(const struct CLMatrix *mat,
       const struct CLDatumList *lab1, const struct CLDatumList *lab2) {
  SETFUNCNAME(CLNewResultMatrix);
  ENSURE_TRUE(mat != NULL, "matrix must not be NULL");
  struct CLCompLearnResult *result = (CLCompLearnResult*)calloc(sizeof(struct CLCompLearnResult),1);
  result->matrix = CLNewMatrixFromMatrix(mat);
  if (lab1 != NULL) {
    result->lab1 = CLNewDatumListFromLabels(lab1);
  }
  if (lab2 != NULL) {
    result->lab2 = CLNewDatumListFromLabels(lab2);
  }

  return result;
}

int CLIsResultNumber(const struct CLCompLearnResult *result) {
  SETFUNCNAME(CLIsResultNumber);
  ENSURE_TRUE(result != NULL, "result cannot be NULL");
  return result->matrix == NULL && result->vector == NULL;
}

CLNumber CLGetNumberResult(const struct CLCompLearnResult *result) {
  SETFUNCNAME(CLGetNumberResult);
  ENSURE_TRUE(CLIsResultNumber(result), "result is not a number");
  return result->number;
}

int CLIsResultVector(const struct CLCompLearnResult *result) {
  SETFUNCNAME(CLIsResultVector);
  ENSURE_TRUE(result != NULL, "result cannot be NULL");
  return result->vector != NULL;
}

int CLIsResultMatrix(const struct CLCompLearnResult *result) {
  SETFUNCNAME(CLIsResultMatrix);
  ENSURE_TRUE(result != NULL, "result cannot be NULL");
  return result->matrix != NULL;
}

void CLFreeResult(struct CLCompLearnResult *result) {
  SETFUNCNAME(CLFreeResult);
  ENSURE_TRUE(result != NULL, "result cannot be NULL");
  do {
    if (result->lab1) {
      CLFreeDatumList(result->lab1);
    }
    if (result->lab2) {
      CLFreeDatumList(result->lab2);
    }
    if (CLIsResultMatrix(result)) {
      CLFreeMatrix(result->matrix);
      break;
    }
    if (CLIsResultVector(result)) {
      CLFreeVector(result->vector);
      break;
    }
  } while (0);
  free(result);
}

const struct CLVector *CLVectorFromResult(const struct CLCompLearnResult *result) {
  SETFUNCNAME(CLVectorFromResult);
  ENSURE_TRUE(result != NULL, "result cannot be NULL");
  ENSURE_TRUE(CLIsResultVector(result), "result is not a vector");
  return result->vector;
}

const struct CLMatrix *CLMatrixFromResult(const struct CLCompLearnResult *result) {
  SETFUNCNAME(CLMatrixFromResult);
  ENSURE_TRUE(result != NULL, "result cannot be NULL");
  ENSURE_TRUE(CLIsResultMatrix(result), "result is not a matrix");
  return result->matrix;
}

struct CLSimpleDatum *CLConcatDatums(const struct CLDatum *a, const struct CLDatum *b) {
  return CLConcatSimpleDatums(CLDatumGetObject(a), CLDatumGetObject(b));
}

static CLNumber compressed_size_for_simple_datum_and_options(const struct CLSimpleDatum *sdatum, const struct CLCommandOptions *opts) {
  if (opts == NULL) {
    opts = &CLCommandDefaultOptions;
  }
  int deltaBytes = opts->compression.deltaBytes;
  const char *buf = (char*)CLGetSimpleDatumBytes(sdatum);
  size_t len = CLGetSimpleDatumSize(sdatum);
  return compressed_size_in_bits_and_delta(buf, len, deltaBytes);
}

static CLNumber compressed_size_for_datum_and_options(const struct CLDatum *obj,
  const struct CLCommandOptions *opts) {
  const struct CLSimpleDatum *sdatum = CLDatumGetObject(obj);
  return compressed_size_for_simple_datum_and_options(sdatum, opts);
}

static int are_simple_datums_equal(const struct CLSimpleDatum *da,
  const struct CLSimpleDatum *db)
{
    int sda = CLGetSimpleDatumSize(da);
    int sdb = CLGetSimpleDatumSize(db);
    if (sda != sdb) { return 0; }
    if (sda > 0) {
      if (memcmp(CLGetSimpleDatumBytes(da), CLGetSimpleDatumBytes(db), sda) != 0) { return 0; }
    }
  return 1;
}

static int are_datums_equal(const struct CLDatum *a, const struct CLDatum *b) {
  const struct CLSimpleDatum *da = CLDatumGetObject(a);
  const struct CLSimpleDatum *db = CLDatumGetObject(b);
  return are_simple_datums_equal(da, db);
}

static int are_datum_lists_equal(const struct CLDatumList *a, const struct CLDatumList *b) {
  size_t sa = CLDatumListSize(a);
  size_t sb = CLDatumListSize(b);
  if (sa != sb) { return 0; }
  size_t i;
  for (i = 0; i < sa; ++i) {
    if (!are_datums_equal(CLGetDatumAt(a,i), CLGetDatumAt(b,i))) {
      return 0;
    }
  }
  return 1;
}

static struct CLDatumList *CLNewDatumListFromLabels(const struct CLDatumList *dl) {
  size_t i;
  size_t size = CLDatumListSize(dl);
  struct CLDatumList *result = CLNewDatumList(size);
  for (i = 0; i < size; ++i) {
    const struct CLDatum *d = CLGetDatumAt(dl, i);
    const struct CLSimpleDatum *label = CLDatumGetLabel(d);
    struct CLSimpleDatum *empty = CLNewSimpleDatum(NULL, 0);
    struct CLDatum *ld = CLNewDatum(empty, label);
    CLSetDatumAt(result, i, ld);
    CLFreeSimpleDatum(empty);
    CLFreeDatum(ld);
  }
  return result;
}

struct CLCompLearnResult *CLCalculate(enum CLBasicCommand cmd, enum CLCalculationType ctype, const struct CLDatumList *a, const struct CLDatumList *b, const struct CLCommandOptions *opts) {
  SETFUNCNAME(CLCalculate);
  ENSURE_TRUE(a != NULL, "first datum list cannot be NULL");
  struct CLCompLearnResult *result = NULL, *resulta = NULL, *resultb = NULL, *resultboth = NULL;
  struct CLSimpleDatum *dab = NULL;
  CLNumber c = 0, ca = 0, cb = 0, cab = 0;
  struct CLVector *vec = NULL;
  const struct CLVector *veca = NULL, *vecb = NULL;
  struct CLMatrix *mat = NULL;
  const struct CLMatrix *submat = NULL;
  size_t i, j, s, w, h, eq;
  switch (cmd) {
    case CLSingle:
      ENSURE_TRUE(a != NULL, "first datum list cannot be NULL");
      ENSURE_TRUE(CLDatumListSize(a)==1, "must have one element in first list");
      ENSURE_TRUE(b == NULL, "second datum list must be NULL");
      ENSURE_TRUE(ctype == CLSize, "ctype must be CLSize for a single object");
      ca = compressed_size_for_datum_and_options(CLGetDatumAt(a, 0), opts);
      result = CLNewResultNumber(ca);
      return result;

    case CLPair:
      ENSURE_TRUE(a != NULL, "first datum list cannot be NULL");
      ENSURE_TRUE(CLDatumListSize(a)==1, "must have one element in first list");
      ENSURE_TRUE(b != NULL, "second datum list cannot be NULL");
      ENSURE_TRUE(CLDatumListSize(b)==1,"must have one element in second list");
      if (are_datums_equal(CLGetDatumAt(a,0), CLGetDatumAt(b,0))) {
        result = CLNewResultNumber(0);
        return result;
      }
      dab = CLConcatDatums(CLGetDatumAt(a,0),CLGetDatumAt(b,0));
      cab = compressed_size_for_simple_datum_and_options(dab, opts);
      CLFreeSimpleDatum(dab);
      if (ctype == CLSize) {
        result = CLNewResultNumber(cab);
        return result;
      }
      if (ctype == CLNCD) {
        ca = compressed_size_for_datum_and_options(CLGetDatumAt(a,0), opts);
        cb = compressed_size_for_datum_and_options(CLGetDatumAt(b,0), opts);
        result = CLNewResultNumber(CLNCDBasic(ca, cb, cab));
        return result;
      }
      ENSURE_TRUE(0,"must have command type CLSize or CLNCD");
      return NULL;

    case CLVector:
      ENSURE_TRUE(a != NULL, "first datum list cannot be NULL");
      ENSURE_TRUE(CLDatumListSize(a)>=1, "must have at least one element in first list");
      ENSURE_TRUE(b == NULL, "second datum list must be NULL");
      ENSURE_TRUE(ctype == CLSize, "ctype must be CLSize for a vector");
      s = CLDatumListSize(a);
      vec = CLNewVector(s);
      for (i = 0; i < s; ++i) {
        c = compressed_size_for_datum_and_options(CLGetDatumAt(a, i), opts);
        CLSetVectorValue(vec, i, c);
      }
      result = CLNewResultVector(vec, a);
      CLFreeVector(vec);
      return result;
    case CLMatrix:
      ENSURE_TRUE(a != NULL, "first datum list cannot be NULL");
      ENSURE_TRUE(CLDatumListSize(a)>=1, "must have at least one element in first list");
      ENSURE_TRUE(b != NULL, "second datum list cannot be NULL");
      ENSURE_TRUE(CLDatumListSize(b)>=1, "must have at least one element in second list");
      w = CLDatumListSize(a);
      h = CLDatumListSize(b);
      eq = are_datum_lists_equal(a, b);
      mat = CLNewMatrix(w,h);
      if (ctype == CLSize) {
        for (i = 0; i < w; ++i) {
          for (j = eq ? i : 0; j < h; ++j) {
            dab = CLConcatDatums(CLGetDatumAt(a,i),CLGetDatumAt(b,j));
            cab = compressed_size_for_simple_datum_and_options(dab, opts);
            CLFreeSimpleDatum(dab);
            CLSetMatrixValue(mat, i, j, cab);
            if (eq) {
              CLSetMatrixValue(mat, j, i, cab);
            }
          }
        }
        result = CLNewResultMatrix(mat, a, b);
        CLFreeMatrix(mat);
        return result;
      }
      ENSURE_TRUE(ctype == CLNCD, "command type must be CLNCD or CLSize");
      resulta = CLCalculate(CLVector, CLSize, a, NULL, opts);
      resultb = CLCalculate(CLVector, CLSize, b, NULL, opts);
      veca = CLVectorFromResult(resulta);
      vecb = CLVectorFromResult(resultb);
      resultboth = CLCalculate(CLMatrix, CLSize, a, b, opts);
      submat = CLMatrixFromResult(resultboth);
      for (i = 0; i < w; ++i) {
        for (j = 0; j < h; ++j) {
            if (are_datums_equal(CLGetDatumAt(a,i),CLGetDatumAt(b,j))) {
              CLSetMatrixValue(mat, i, j, 0);
            } else {
              CLSetMatrixValue(mat, i, j, CLNCDBasic(
                CLGetVectorValue(veca,i),
                CLGetVectorValue(vecb,j),
                CLGetMatrixValue(submat, i, j)));
            }
        }
      }
      result = CLNewResultMatrix(mat, a, b);
      CLFreeMatrix(mat);
      CLFreeResult(resulta);
      CLFreeResult(resultb);
      CLFreeResult(resultboth);
      return result;
    default:
      ENSURE_TRUE(0, "command must be CLSingle, CLPair, CLVector, or CLMatrix");
      return NULL;
  }
  // CLSize CLNCD

}
#endif
