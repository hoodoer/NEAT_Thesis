#ifndef __CL_COMMAND_H
#define __CL_COMMAND_H

struct CLCommandOptions;
struct CLCompLearnResult;

enum CLBasicCommand {
  CLSingle,
  CLPair,
  CLVector,
  CLMatrix
};

enum CLCalculationType {
  CLSize,
  CLNCD
};

struct CLCompLearnResult *CLCalculate(enum CLBasicCommand cmd, enum CLCalculationType ctype, const struct CLDatumList *a, const struct CLDatumList *b, const struct CLCommandOptions *opts);

#endif
