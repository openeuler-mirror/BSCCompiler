//
// Created by wchenbt on 9/5/2021.
//

#ifndef MAPLE_SAN_UBSAN_BOUNDS_H
#define MAPLE_SAN_UBSAN_BOUNDS_H
#include "me_function.h"
#include "me_phase_manager.h"
#include "mir_builder.h"
#include "mir_module.h"

namespace maple {

class ArrayInfo {
 public:
  StmtNode *usedStmt;
  MIRArrayType *arrayType;
  size_t neededSize;
  std::vector<MIRType *> elemType;
  std::vector<BaseNode *> offset;
  std::vector<size_t> dimensions;
  std::vector<std::array<CompareNode *, 3>> checks;

  ArrayInfo(StmtNode *usedStmt, MIRArrayType *arrayType, ArrayNode *arrayNode);
  size_t GetElementSize();
  void SetNeededSize(size_t neededSize);
  std::string GetArrayTypeName(size_t dim);
};

class BoundCheck {
 public:
  BoundCheck(MeFunction *func);
  bool addBoundsChecking();
  void initializeCallbacks();
  void insertBoundsCheck(ArrayInfo *arrayInfo, size_t dim);
  void getBoundsCheckCond(ArrayInfo *arrayInfo, BlockNode *body, size_t dim);

  std::vector<ArrayInfo> getArrayInfo(StmtNode *stmtNode);

  MeFunction *func;
  MIRBuilder *mirBuilder;
  MIRModule *mirModule;
  MIRStructType *sourceLocType;
  MIRStructType *typeDescriptor;
  MIRStructType *outofBoundsData;
  MIRFunction *ubsanHandler;

  MIRSymbol *symbol_1;
  MIRSymbol *symbol_2;
  MIRSymbol *outofBound;
  MIRSymbol *sourceLoc;
  MIRSymbol *arrayType;
  MIRSymbol *indexType;
};
}  // namespace maple
#endif  // MAPLE_SAN_UBSAN_BOUNDS_H
