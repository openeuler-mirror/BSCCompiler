//
// Created by wchenbt on 9/5/2021.
//

#ifndef MAPLE_SAN_UBSAN_BOUNDS_H
#define MAPLE_SAN_UBSAN_BOUNDS_H
#include "me_function.h"
#include "mir_module.h"
#include "me_phase_manager.h"
#include "mir_builder.h"


namespace maple {

  class ArrayInfo {
    public:
      StmtNode *usedStmt;
      MIRArrayType *arrayType;
      uint64_t neededSize;
      std::vector<MIRType *> elemType;
      std::vector<BaseNode *> offset;
      std::vector<uint16> dimensions;
      std::vector<std::array<CompareNode *, 3>> checks;

      ArrayInfo(StmtNode *usedStmt, MIRArrayType *arrayType, ArrayNode *arrayNode);
      uint64_t GetElementSize();
      void SetNeededSize(uint64 neededSize);
      std::string GetArrayTypeName(int dim);
  };

  class BoundCheck {
  public:
    BoundCheck(MeFunction *func);
    bool addBoundsChecking();
    void initializeCallbacks();
    void insertBoundsCheck(ArrayInfo *arrayInfo, int dim);
    void getBoundsCheckCond(ArrayInfo *arrayInfo, BlockNode *body, int dim);

    std::vector<ArrayInfo> getArrayInfo(StmtNode *stmtNode);

    MeFunction *func;
    MIRBuilder *mirBuilder;
    MIRModule *mirModule;
    MIRStructType *sourceLocType;
    MIRStructType *typeDescriptor;
    MIRStructType *outofBoundsData;
    MIRFunction *ubsanHandler;

    MIRSymbol *symbol_1, *symbol_2;
    MIRSymbol *outofBound, *sourceLoc, *arrayType, *indexType;
  };
}
#endif //MAPLE_SAN_UBSAN_BOUNDS_H
