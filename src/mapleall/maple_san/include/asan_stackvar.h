//
// Created by wchenbt on 3/4/2021.
//
#ifdef ENABLE_MAPLE_SAN

#ifndef MAPLE_SAN_ASAN_STACKVAR_H
#define MAPLE_SAN_ASAN_STACKVAR_H

#include <set>

#include "asan_function.h"
#include "asan_interfaces.h"
#include "me_phase_manager.h"
#include "module_phase_manager.h"
#include "san_common.h"

namespace maple {

class FunctionStackPoisoner {
 public:
  FunctionStackPoisoner(MeFunction &function, AddressSanitizer &asan);

  bool runOnFunction();

  void processStackVariable();

  void unpoisonDynamicAllocas();

  void initializeCallbacks(const MIRModule &M);

  void createDynamicAllocasInitStorage();

  bool isInFirstBlock(StmtNode *stmtNode);

  BaseNode *GetTransformedNode(MIRSymbol *oldVar, MIRSymbol *newVar, BaseNode *baseNode);

  void replaceAllUsesWith(MIRSymbol *oldVar, MIRSymbol *newVar);

  void handleDynamicAllocaCall(ASanDynaVariableDescription *AI);

  MIRSymbol *createAllocaForLayout(StmtNode *insBefore, MIRBuilder *mirBuilder, const ASanStackFrameLayout &L);

  void unpoisonDynamicAllocasBeforeInst(StmtNode *InstBefore);

  void copyToShadow(const std::vector<uint8_t> ShadowMask, const std::vector<uint8_t> ShadowBytes, MIRBuilder *mirBuilder,
                    BaseNode *ShadowBase, StmtNode *InsBefore);

  void copyToShadow(const std::vector<uint8_t> ShadowMask, const std::vector<uint8_t> ShadowBytes, size_t Begin, size_t End,
                    MIRBuilder *mirBuilder, BaseNode *ShadowBase, StmtNode *InsBefore);

  void copyToShadowInline(const std::vector<uint8_t> ShadowMask, const std::vector<uint8_t> ShadowBytes, size_t Begin, size_t End,
                          MIRBuilder *mirBuilder, BaseNode *ShadowBase, StmtNode *InsBefore);

  bool isFuncCallArg(const MIRSymbol *const symbolPtr) const;
  bool isFuncCallArg(const std::string symbolName) const;

  std::set<MIRSymbol *> GetStackVarReferedByCallassigned();

  AddressSanitizer &ASan;

  MeFunction *meFunction;
  MIRFunction *mirFunction;

  MIRModule *module;
  MIRType *IntptrTy;
  MIRType *IntptrPtrTy;
  ShadowMapping Mapping;

  unsigned StackAlignment;
  MIRSymbol *DynamicAllocaLayout = nullptr;

  std::vector<StmtNode *> RetVec;
  std::vector<ASanStackVariableDescription> stackVariableDesc;
  std::vector<ASanDynaVariableDescription> dynamicAllocaDesc;
  std::set<const MIRSymbol *> callArgSymbols;
  std::set<std::string> callArgSymbolNames;

  MIRFunction *AsanSetShadowFunc[0x100] = {};
  MIRFunction *AsanAllocaPoisonFunc, *AsanAllocasUnpoisonFunc;

  bool HasNonEmptyInlineAsm = false;
  bool HasReturnsTwiceCall = false;

  std::map<MIRSymbol *, bool> isUsedInAlloca;
 private:
  void collectLocalVariablesWithoutAlloca();
  void collectLocalVariablesWithAlloca();
  void collectDescFromUnaryStmtNode(UnaryStmtNode &assignNode);
};
}  // namespace maple
#endif  // MAPLE_SAN_ASAN_STACKVAR_H

#endif