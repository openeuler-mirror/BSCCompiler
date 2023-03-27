//
// Created by wchenbt on 4/4/2021.
//

#ifndef MAPLE_SAN_ASAN_FUNCTION_H
#define MAPLE_SAN_ASAN_FUNCTION_H

#include "asan_mapping.h"
#include "me_cfg.h"
#include "me_function.h"
#include "me_ssa.h"
#include "mir_module.h"
#include "san_common.h"

namespace maple {

struct MemoryAccess {
  StmtNode *stmtNode;
  bool isWrite;
  uint64_t typeSize;
  size_t alignment;
  BaseNode *ptrOperand;
};

// Accesses sizes are powers of two: 1, 2, 4, 8, 16.
class AddressSanitizer {
 public:
  AddressSanitizer(MIRModule &module, PreAnalysis *symbolInteresting)
      : module(&module), Mapping(getShadowMapping()), preAnalysis(symbolInteresting) {
    LongSize = kSizeOfPtr * 8;
    IntPtrPrim = LongSize == sizeof(int32) ? PTY_i32 : PTY_i64;
    IntPtrTy = GlobalTables::GetTypeTable().GetPrimType(IntPtrPrim);
  }

  bool instrumentFunction(MeFunction &F);

  void instrumentAddress(StmtNode *OrigIns, StmtNode *InsertBefore, BaseNode *Addr, uint64_t TypeSize, bool IsWrite,
                         BaseNode *SizeArgument);

  void instrumentUnusualSizeOrAlignment(StmtNode *I, StmtNode *InsertBefore, BaseNode *Addr, uint64_t TypeSize,
                                        bool IsWrite);

 private:
  friend class FunctionStackPoisoner;

  void instrumentMop(StmtNode *I, std::vector<MemoryAccess> &memoryAccess);

  void initializeCallbacks(const MIRModule &mirModule);

  bool isInterestingSymbol(const MIRSymbol &symbol);

  bool isInterestingAlloca(const UnaryNode &unaryNode);

  void instrumentMemIntrinsic(IntrinsiccallNode *stmtNode);

  void maybeInsertDynamicShadowAtFunctionEntry(const MeFunction &F);

  BaseNode *memToShadow(BaseNode *Shadow, MIRBuilder &mirBuilder);

  /// If it is an interesting memory access, return the PointerOperand
  /// and set IsWrite/Alignment. Otherwise return nullptr.
  std::vector<MemoryAccess> isInterestingMemoryAccess(StmtNode *stmtNode);

  MemoryAccess getIassignMemoryAccess(IassignNode &iassign);

  MemoryAccess getIassignoffMemoryAccess(IassignoffNode &iassignoff);

  MemoryAccess getIreadMemoryAccess(IreadNode &iread, StmtNode *stmtNode);

  StmtNode *splitIfAndElseBlock(Opcode op, StmtNode *elsePart, const BinaryNode *cmpStmt);

  CallNode *generateCrashCode(MIRSymbol *Addr, bool IsWrite, size_t AccessSizeIndex, BaseNode *SizeArgument);

  BinaryNode *createSlowPathCmp(StmtNode *InsBefore, BaseNode *AddrLong, BaseNode *ShadowValue, uint64_t TypeSize);

  void SanrazorProcess(MeFunction &mefunc, std::set<StmtNode *> &userchecks,
                       std::map<uint32, std::vector<StmtNode *>> &brgoto_map, std::map<uint32, uint32> &stmt_to_bbID,
                       std::map<int, StmtNode *> &stmt_id_to_stmt, std::vector<int> &stmt_id_list, int check_env);

  struct FunctionStateRAII {
    AddressSanitizer *Phase;

    FunctionStateRAII(AddressSanitizer *Phase) : Phase(Phase) {
      assert(Phase->ProcessedSymbols.empty() && "last pass forgot to clear cache");
      assert(!Phase->LocalDynamicShadow);
    }

    ~FunctionStateRAII() {
      Phase->LocalDynamicShadow = nullptr;
      Phase->ProcessedSymbols.clear();
      Phase->preAnalysis->usedInAddrof.clear();
    }
  };

  size_t LongSize;
  MeFunction *func;
  MIRModule *module;
  PrimType IntPtrPrim;
  MIRType *IntPtrTy;
  ShadowMapping Mapping;
  MIRFunction *AsanHandleNoReturnFunc;

  // These arrays is indexed by AccessIsWrite and log2(AccessSize).
  MIRFunction *AsanErrorCallback[2][kNumberOfAccessSizes];
  // These arrays is indexed by AccessIsWrite
  MIRFunction *AsanErrorCallbackSized[2];

  MIRFunction *AsanMemmove, *AsanMemcpy, *AsanMemset;
  MIRFunction *AsanRBTSafetyCheck, *AsanRBTStackInsert, *AsanRBTStackDelete, *AsanRBTPoisonRegion,
      *AsanRBTUnpoisonRegion;

  MIRFunction *__san_cov_flush = nullptr;

  BaseNode *LocalDynamicShadow = nullptr;

  std::map<const MIRSymbol * const, bool> ProcessedSymbols;
  std::map<const UnaryNode * const, bool> ProcessedAllocas;

  PreAnalysis *preAnalysis;
};

}  // namespace maple
#endif  // MAPLE_SAN_ASAN_FUNCTION_H
