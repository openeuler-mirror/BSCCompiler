//
// Created by wchenbt on 4/4/2021.
//

#ifndef MAPLE_SAN_ASAN_FUNCTION_H
#define MAPLE_SAN_ASAN_FUNCTION_H

#include "san_common.h"
#include "me_function.h"
#include "asan_mapping.h"
#include "mir_module.h"
#include "me_cfg.h"
#include "me_ssa.h"

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
        : module(&module), Mapping(getShadowMapping()), preAnalysis(symbolInteresting){
      LongSize = kSizeOfPtr * 8;
      IntPtrPrim = LongSize == sizeof(int32) ? PTY_i32 : PTY_i64;
      IntPtrTy = GlobalTables::GetTypeTable().GetPrimType(IntPtrPrim);
    }

    bool instrumentFunction(MeFunction &F);

    void instrumentAddress(StmtNode *OrigIns, StmtNode *InsertBefore, BaseNode *Addr,
                           uint32_t TypeSize, bool IsWrite, BaseNode *SizeArgument);

    void instrumentUnusualSizeOrAlignment(StmtNode *I, StmtNode *InsertBefore, BaseNode *Addr,
                                          uint32_t TypeSize, bool IsWrite);

  private:
    friend class FunctionStackPoisoner;

    void instrumentMop(StmtNode *I);

    void initializeCallbacks(MIRModule &module);

    bool isInterestingSymbol(MIRSymbol &symbol);

    bool isInterestingAlloca(UnaryNode &unaryNode);

    void instrumentMemIntrinsic(IntrinsiccallNode *stmtNode);

    void maybeInsertDynamicShadowAtFunctionEntry(MeFunction &F);

    BaseNode *memToShadow(BaseNode* Shadow, MIRBuilder &mirBuilder);

    /// If it is an interesting memory access, return the PointerOperand
    /// and set IsWrite/Alignment. Otherwise return nullptr.
    std::vector<MemoryAccess> isInterestingMemoryAccess(StmtNode *stmtNode);

    StmtNode* splitIfAndElseBlock(Opcode op, StmtNode *elsePart, const BinaryNode *cmpStmt);

    CallNode* generateCrashCode(MIRSymbol* Addr, bool IsWrite, size_t AccessSizeIndex, BaseNode *SizeArgument);

    BinaryNode *createSlowPathCmp(StmtNode *InsBefore,BaseNode *AddrLong, BaseNode *ShadowValue, uint32_t TypeSize);

    struct FunctionStateRAII {
      AddressSanitizer *Phase;

      FunctionStateRAII(AddressSanitizer *Phase) : Phase(Phase) {
        assert(Phase->ProcessedSymbols.empty() &&
               "last pass forgot to clear cache");
        assert(!Phase->LocalDynamicShadow);
      }

      ~FunctionStateRAII() {
        Phase->LocalDynamicShadow = nullptr;
        Phase->ProcessedSymbols.clear();
        Phase->preAnalysis->usedInAddrof.clear();
      }
    };

    int LongSize;
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
    MIRFunction *AsanRBTSafetyCheck, *AsanRBTStackInsert, *AsanRBTStackDelete,
                *AsanRBTPoisonRegion, *AsanRBTUnpoisonRegion;
                
    MIRFunction *__san_cov_flush = nullptr;    

    BaseNode* LocalDynamicShadow = nullptr;

    std::map<MIRSymbol *, bool> ProcessedSymbols;
    std::map<UnaryNode *, bool> ProcessedAllocas;

    PreAnalysis *preAnalysis;
  };

} // end namespace maple
#endif //MAPLE_SAN_ASAN_FUNCTION_H
