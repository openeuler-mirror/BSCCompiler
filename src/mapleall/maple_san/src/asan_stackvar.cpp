//
// Created by wchenbt on 3/4/2021.
//

#include "asan_stackvar.h"

#include <string_utils.h>

#include "san_common.h"
#include <set>

#define ENABLE_STACK_SIZE_LIMIT false

namespace maple {

// Fake stack allocator (asan_fake_stack.h) has 11 size classes
// for every power of 2 from kMinStackMallocSize to kMaxAsanStackMallocSizeClass
static int StackMallocSizeClass(uint64_t LocalStackSize) {
  // unblock the stack size limit
  if (ENABLE_STACK_SIZE_LIMIT) {
    CHECK_FATAL(LocalStackSize <= kMaxStackMallocSize, "Too large stack size.");
  }
  uint64_t MaxSize = kMinStackMallocSize;
  for (int i = 0;; i++, MaxSize *= 2) {
    if (LocalStackSize <= MaxSize) {
      return i;
    }
  }
}

FunctionStackPoisoner::FunctionStackPoisoner(MeFunction &function, AddressSanitizer &asan)
    : ASan(asan),
      meFunction(&function),
      mirFunction(function.GetMirFunc()),
      module(mirFunction->GetModule()),
      IntptrTy(asan.IntPtrTy),
      Mapping(asan.Mapping),
      StackAlignment(1 << Mapping.Scale) {
  IntptrPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(*IntptrTy, PTY_ptr);
  // initialize function arguments symbols
  for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
    const MIRSymbol *symbolPtr = mirFunction->GetFormal(i);
    callArgSymbols.insert(symbolPtr);
    callArgSymbolNames.insert(symbolPtr->GetName());
  }
}

void FunctionStackPoisoner::copyToShadow(std::vector<uint8_t> ShadowMask, std::vector<uint8_t> ShadowBytes,
                                         MIRBuilder *mirBuilder, BaseNode *ShadowBase, StmtNode *InsBefore) {
  copyToShadow(ShadowMask, ShadowBytes, 0, ShadowMask.size(), mirBuilder, ShadowBase, InsBefore);
}

void FunctionStackPoisoner::copyToShadow(std::vector<uint8_t> ShadowMask, std::vector<uint8_t> ShadowBytes,
                                         size_t Begin, size_t End, MIRBuilder *mirBuilder, BaseNode *ShadowBase,
                                         StmtNode *InsBefore) {
  assert(ShadowMask.size() == ShadowBytes.size());
  size_t Done = Begin;
  const size_t threshold = 64;
  for (size_t i = Begin, j = Begin + 1; i < End; i = j++) {
    if (!ShadowMask[i]) {
      assert(!ShadowBytes[i]);
      continue;
    }
    uint8_t Val = ShadowBytes[i];
    if (!AsanSetShadowFunc[Val]) {
      continue;
    }
    // Skip same values.
    while (j < End && ShadowMask[j] && Val == ShadowBytes[j]) {
      ++j;
    }

    if (j - i >= threshold) {
      copyToShadowInline(ShadowMask, ShadowBytes, Done, i, mirBuilder, ShadowBase, InsBefore);
      Done = j;
    }
  }

  copyToShadowInline(ShadowMask, ShadowBytes, Done, End, mirBuilder, ShadowBase, InsBefore);
}

void FunctionStackPoisoner::copyToShadowInline(std::vector<uint8_t> ShadowMask, std::vector<uint8_t> ShadowBytes,
                                               size_t Begin, size_t End, MIRBuilder *mirBuilder, BaseNode *ShadowBase,
                                               StmtNode *InsBefore) {
  if (Begin >= End) {
    return;
  }

  const size_t LargestStoreSizeInBytes = std::min<size_t>(sizeof(uint64_t), ASan.LongSize / 8);

  for (size_t i = Begin; i < End;) {
    if (!ShadowMask[i]) {
      assert(!ShadowBytes[i]);
      ++i;
      continue;
    }

    size_t StoreSizeInBytes = LargestStoreSizeInBytes;
    // Fit store size into the range.
    while (StoreSizeInBytes > End - i) {
      StoreSizeInBytes /= 2;
    }

    // Minimize store size by trimming trailing zeros.
    for (size_t j = StoreSizeInBytes - 1; j && !ShadowMask[i + j]; --j) {
      while (j <= StoreSizeInBytes / 2) {
        StoreSizeInBytes /= 2;
      }
    }

    uint64_t Val = 0;
    for (size_t j = 0; j < StoreSizeInBytes; j++) {
      Val |= (uint64_t)ShadowBytes[i + j] << (8 * j);
    }

    BinaryNode *Ptr = mirBuilder->CreateExprBinary(OP_add, *IntptrTy, ShadowBase,
                                                   mirBuilder->CreateIntConst(i, IntptrTy->GetPrimType()));
    PrimType primType;
    switch (StoreSizeInBytes * 8) {
      case 8:
        primType = PTY_i8;
        break;
      case 16:
        primType = PTY_i16;
        break;
      case 32:
        primType = PTY_i32;
        break;
      case 64:
        primType = PTY_i64;
        break;
      default: {
        primType = PTY_unknown;
      }
    }
    ConstvalNode *Poison = mirBuilder->CreateIntConst(Val, primType);
    MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(
        GlobalTables::GetTypeTable().GetPrimType(primType)->GetTypeIndex());
    IassignNode *iassignNode = mirBuilder->CreateStmtIassign(*ptrType, 0, Ptr, Poison);
    iassignNode->InsertAfterThis(*InsBefore);
    i += StoreSizeInBytes;
  }
}

void FunctionStackPoisoner::initializeCallbacks(const MIRModule &M) {
  MIRBuilder *mirBuilder = M.GetMIRBuilder();
#ifdef ENABLERBTREE
#else
  for (size_t Val : {0x00, 0xf1, 0xf2, 0xf3, 0xf5, 0xf8}) {
    std::ostringstream Name;
    Name << kAsanSetShadowPrefix;
    Name << std::setw(2) << std::setfill('0') << std::hex << Val;
    AsanSetShadowFunc[Val] = getOrInsertFunction(mirBuilder, Name.str().c_str(), GlobalTables::GetTypeTable().GetVoid(),
                                                 {IntptrTy, IntptrTy});
  }
#endif
  AsanAllocaPoisonFunc =
      getOrInsertFunction(mirBuilder, kAsanAllocaPoison, GlobalTables::GetTypeTable().GetVoid(), {IntptrTy, IntptrTy});
  AsanAllocasUnpoisonFunc = getOrInsertFunction(mirBuilder, kAsanAllocasUnpoison,
                                                GlobalTables::GetTypeTable().GetVoid(), {IntptrTy, IntptrTy});
}

std::set<MIRSymbol *> FunctionStackPoisoner::GetStackVarReferedByCallassigned() {
  std::set<MIRSymbol *> retOfCallassigned;
  MIRSymbolTable *symbolTable = mirFunction->GetSymTab();
  for (StmtNode &stmt : mirFunction->GetBody()->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_callassigned) {
      CallNode *tmp = dynamic_cast<CallNode *>(&stmt);
      const CallReturnVector *retVecPtr = tmp->GetCallReturnVector();
      // get the return value being refered by a function call
      for (const CallReturnPair &retPair : *retVecPtr) {
        MIRSymbol *symbol = symbolTable->GetSymbolFromStIdx(retPair.first.Idx());
        retOfCallassigned.insert(symbol);
      }
    }
  }
  return retOfCallassigned;
}

bool FunctionStackPoisoner::runOnFunction() {
  initializeCallbacks(*module);
  // Collect all variables refered by return statement of callassigned
  std::set<MIRSymbol *> retOfCallassigned = GetStackVarReferedByCallassigned();
  // Collect alloca, ret, etc.
  for (StmtNode &stmt : mirFunction->GetBody()->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_return) {
      RetVec.push_back(&stmt);
    }
  }
  // Collect local variable
  MIRSymbolTable *symbolTable = mirFunction->GetSymTab();
  size_t size = symbolTable->GetSymbolTableSize();
  for (size_t i = 0; i < size; ++i) {
    MIRSymbol *symbol = symbolTable->GetSymbolFromStIdx(i);
    if (symbol == nullptr) {
      continue;
    }
    if (symbol->IsDeleted() || symbol->GetName() == "") {
      continue;
    }
    if (ASan.isInterestingSymbol(*symbol)) {
      if (StringUtils::StartsWith(symbol->GetName(), "asan_")) {
        continue;
      }
      // we skip symbols being refered by callassigned as a return value
      if (retOfCallassigned.find(symbol) != retOfCallassigned.end()) {
        continue;
      }
      StackAlignment = std::max(StackAlignment, symbol->GetType()->GetAlign());
      ASanStackVariableDescription description = {symbol->GetName(),
                                                  symbol->GetType()->GetSize(),
                                                  0,
                                                  symbol->GetType()->GetAlign(),
                                                  symbol,
                                                  nullptr,
                                                  0,
                                                  symbol->GetSrcPosition().LineNum()};
      stackVariableDesc.push_back(description);
    }
  }
  for (StmtNode &stmt : mirFunction->GetBody()->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_regassign || stmt.GetOpCode() == OP_dassign) {
      UnaryStmtNode *assignNode = dynamic_cast<UnaryStmtNode *>(&stmt);
      BaseNode *baseNode = assignNode->GetRHS();

      while (baseNode) {
        if (UnaryNode *rhs = dynamic_cast<UnaryNode *>(baseNode)) {
          if (rhs->GetOpCode() == OP_alloca && ASan.isInterestingAlloca(*rhs)) {
            ConstvalNode *constvalNode = dynamic_cast<ConstvalNode *>(rhs->Opnd(0));

            if (constvalNode && isInFirstBlock(&stmt)) {
              // static alloca
              MIRIntConst *mirConst = dynamic_cast<MIRIntConst *>(constvalNode->GetConstVal());
              ASanStackVariableDescription description = {
                  "",      static_cast<size_t>(mirConst->GetValue().GetZXTValue()),
                  0,       0,
                  nullptr, &stmt,
                  0,       stmt.GetSrcPos().LineNum()};
              stackVariableDesc.push_back(description);
            } else {
              // dynamic alloca
              ASanDynaVariableDescription description = {"", rhs->Opnd(0), &stmt, 0, stmt.GetSrcPos().LineNum()};
              dynamicAllocaDesc.push_back(description);
            }
          }
          baseNode = rhs->Opnd(0);
        } else {
          baseNode = nullptr;
        }
      }
    }
  }
  auto iter = stackVariableDesc.begin();
  while (iter != stackVariableDesc.end()) {
    if (iter->Symbol != nullptr && isUsedInAlloca[iter->Symbol]) {
      iter = stackVariableDesc.erase(iter);
    } else {
      ++iter;
    }
  }
  if (stackVariableDesc.empty() && dynamicAllocaDesc.empty()) {
    return false;
  }

  if (!dynamicAllocaDesc.empty()) {
    createDynamicAllocasInitStorage();
    for (auto &AI : dynamicAllocaDesc) {
      handleDynamicAllocaCall(&AI);
    }
    unpoisonDynamicAllocas();
  }
  processStackVariable();
  return true;
}

bool FunctionStackPoisoner::isInFirstBlock(StmtNode *stmtNode) {
  while (stmtNode) {
    if (stmtNode->IsCondBr()) {
      CondGotoNode *condGotoNode = dynamic_cast<CondGotoNode *>(stmtNode);
      ConstvalNode *constvalNode = dynamic_cast<ConstvalNode *>(condGotoNode->Opnd(0));
      if (constvalNode) {
        MIRIntConst *mirIntConst = dynamic_cast<MIRIntConst *>(constvalNode->GetConstVal());
        if (mirIntConst && mirIntConst->GetValue() == 1) {
          stmtNode = stmtNode->GetPrev();
          continue;
        }
      }
      return false;
    }
    stmtNode = stmtNode->GetPrev();
  }
  return true;
}

void FunctionStackPoisoner::createDynamicAllocasInitStorage() {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  DynamicAllocaLayout = getOrCreateSymbol(mirBuilder, IntptrTy->GetTypeIndex(), "asan_dynamic_alloca", kStVar, kScAuto,
                                          mirFunction, kScopeLocal);
  DynamicAllocaLayout->GetAttrs().SetAlign(32);
  DassignNode *dassignNode = mirBuilder->CreateStmtDassign(DynamicAllocaLayout->GetStIdx(), 0,
                                                           mirBuilder->CreateIntConst(0, IntptrTy->GetPrimType()));
  mirFunction->GetBody()->InsertBefore(mirFunction->GetBody()->GetFirst(), dassignNode);
}

void FunctionStackPoisoner::unpoisonDynamicAllocasBeforeInst(StmtNode *InstBefore, MIRSymbol *SavedStack) {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.emplace_back(mirBuilder->CreateDread(*DynamicAllocaLayout, IntptrTy->GetPrimType()));
  args.emplace_back(mirBuilder->CreateAddrof(*DynamicAllocaLayout, PTY_u64));

  CallNode *callNode = mirBuilder->CreateStmtCall(AsanAllocasUnpoisonFunc->GetPuidx(), args);
  callNode->InsertAfterThis(*InstBefore);
}

// Unpoison dynamic allocas redzones.
void FunctionStackPoisoner::unpoisonDynamicAllocas() {
  for (auto &Ret : RetVec) {
    unpoisonDynamicAllocasBeforeInst(Ret, DynamicAllocaLayout);
  }
}

void FunctionStackPoisoner::handleDynamicAllocaCall(ASanDynaVariableDescription *AI) {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  const unsigned Align = std::max(kAllocaRzSize, (unsigned int)1);
  const uint64_t AllocaRedzoneMask = kAllocaRzSize - 1;

  ConstvalNode *Zero = mirBuilder->CreateIntConst(0, IntptrTy->GetPrimType());
  ConstvalNode *AllocaRzSize = mirBuilder->CreateIntConst(kAllocaRzSize, IntptrTy->GetPrimType());
  ConstvalNode *AllocaRzMask = mirBuilder->CreateIntConst(AllocaRedzoneMask, IntptrTy->GetPrimType());

  BaseNode *NewSize;
  if (AI->Size->GetOpCode() == OP_constval) {
    ConstvalNode *constvalNode = dynamic_cast<ConstvalNode *>(AI->Size);
    MIRIntConst *intConst = dynamic_cast<MIRIntConst *>(constvalNode->GetConstVal());
    int PartialSize = intConst->GetValue().GetExtValue() & AllocaRedzoneMask;
    int MisAlign = kAllocaRzSize - PartialSize;
    int PartialPadding = MisAlign;
    if (kAllocaRzSize == MisAlign) {
      PartialPadding = 0;
    }
    int AdditionalChunkSize = Align + kAllocaRzSize + PartialPadding;
    NewSize =
        mirBuilder->CreateIntConst(AdditionalChunkSize + intConst->GetValue().GetExtValue(), IntptrTy->GetPrimType());
  } else {
    BaseNode *OldSize = AI->Size;
    BinaryNode *PartialSize = mirBuilder->CreateExprBinary(OP_band, *IntptrTy, OldSize, AllocaRzMask);
    BaseNode *Misalign = mirBuilder->CreateExprBinary(OP_sub, *IntptrTy, AllocaRzSize, PartialSize);

    MIRSymbol *misAlignSym = getOrCreateSymbol(mirBuilder, IntptrTy->GetTypeIndex(), "asan_misAlign", kStVar, kScAuto,
                                               mirFunction, kScopeLocal);
    DassignNode *dassignNode = mirBuilder->CreateStmtDassign(misAlignSym->GetStIdx(), 0, Misalign);
    dassignNode->InsertAfterThis(*AI->AllocaInst);

    Misalign = mirBuilder->CreateDread(*misAlignSym, IntptrTy->GetPrimType());
    BinaryNode *Cond = mirBuilder->CreateExprCompare(OP_ne, *IntptrTy, *IntptrTy, Misalign, AllocaRzSize);
    TernaryNode *PartialPadding = mirBuilder->CreateExprTernary(OP_select, *IntptrTy, Cond, Misalign, Zero);
    // Align is added to locate left redzone, PartialPadding for possible
    // partial redzone and kAllocaRzSize for right redzone respectively.
    BinaryNode *AdditionalChunkSize = mirBuilder->CreateExprBinary(
        OP_add, *IntptrTy, mirBuilder->CreateIntConst(Align + kAllocaRzSize, IntptrTy->GetPrimType()), PartialPadding);

    NewSize = mirBuilder->CreateExprBinary(OP_add, *IntptrTy, OldSize, AdditionalChunkSize);
  }
  // Insert new alloca with new NewSize and Align params.
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*GlobalTables::GetTypeTable().GetInt8());
  MIRSymbol *tmpAlloca =
      getOrCreateSymbol(mirBuilder, ptrType->GetTypeIndex(), "asan_dyn_tmp", kStVar, kScAuto, mirFunction, kScopeLocal);
  UnaryNode *NewAlloca = mirBuilder->CreateExprUnary(OP_alloca, *GlobalTables::GetTypeTable().GetAddr64(), NewSize);

  DassignNode *dassignNode = mirBuilder->CreateStmtDassign(tmpAlloca->GetStIdx(), 0, NewAlloca);
  dassignNode->InsertAfterThis(*AI->AllocaInst);
  assert(AI->AllocaInst->Opnd(0)->GetOpCode() == OP_alloca);

  BinaryNode *NewAddress = mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetPrimType(PTY_u64),
                                                        mirBuilder->CreateDread(*tmpAlloca, PTY_a64),
                                                        mirBuilder->CreateIntConst(Align, PTY_u64));
  AI->AllocaInst->SetOpnd(NewAddress, 0);

  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.emplace_back(NewAddress);
  args.emplace_back(AI->Size);
  CallNode *callNode = mirBuilder->CreateStmtCall(AsanAllocaPoisonFunc->GetPuidx(), args);
  callNode->InsertAfterThis(*AI->AllocaInst);

  // Insert __asan_alloca_poison call for new created alloca.
  dassignNode = mirBuilder->CreateStmtDassign(
      DynamicAllocaLayout->GetStIdx(), 0,
      mirBuilder->CreateExprTypeCvt(OP_cvt, PTY_i64, PTY_u64, *mirBuilder->CreateDread(*tmpAlloca, PTY_a64)));
  dassignNode->InsertAfterThis(*AI->AllocaInst);
}

MIRSymbol *FunctionStackPoisoner::createAllocaForLayout(StmtNode *insBefore, MIRBuilder *mirBuilder,
                                                        const ASanStackFrameLayout &L) {
  MIRArrayType *arrayType =
      GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetInt8(), L.FrameSize);
  MIRSymbol *tmp =
      getOrCreateSymbol(mirBuilder, arrayType->GetTypeIndex(), "asan_tmp", kStVar, kScAuto, mirFunction, kScopeLocal);
  size_t realignStack = 32;
  assert((realignStack & (realignStack - 1)) == 0);
  size_t frameAlignment = std::max(L.FrameAlignment, realignStack);
  tmp->GetAttrs().SetAlign(frameAlignment);
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*GlobalTables::GetTypeTable().GetInt8());
  MIRSymbol *alloca =
      getOrCreateSymbol(mirBuilder, ptrType->GetTypeIndex(), "asan_alloca", kStVar, kScAuto, mirFunction, kScopeLocal);
  DassignNode *dassignNode =
      mirBuilder->CreateStmtDassign(alloca->GetStIdx(), 0, mirBuilder->CreateAddrof(*tmp, PTY_u64));
  mirFunction->GetBody()->InsertBefore(insBefore, dassignNode);
  return alloca;
}

bool FunctionStackPoisoner::isFuncCallArg(const MIRSymbol *const symbolPtr) const {
  auto iter = callArgSymbols.find(symbolPtr);
  return iter != callArgSymbols.end();
}

bool FunctionStackPoisoner::isFuncCallArg(const std::string symbolName) const {
  auto iter = callArgSymbolNames.find(symbolName);
  return iter != callArgSymbolNames.end();
}

void FunctionStackPoisoner::processStackVariable() {
  if (stackVariableDesc.empty()) {
    return;
  }
  StmtNode *insBefore = mirFunction->GetBody()->GetFirst();
  size_t granularity = 1ULL << Mapping.Scale;
  size_t minHeaderSize = std::max(ASan.LongSize / 2, granularity);
  const ASanStackFrameLayout &L = ComputeASanStackFrameLayout(stackVariableDesc, granularity, minHeaderSize);
  auto descriptionString = ComputeASanStackFrameDescription(stackVariableDesc);
  LogInfo::MapleLogger() << descriptionString << " --- " << L.FrameSize << "\n";

  bool doStackMalloc = true;
  uint64_t localStackSize = L.FrameSize;
  if (ENABLE_STACK_SIZE_LIMIT) {
    doStackMalloc = localStackSize <= kMaxStackMallocSize;
    int stackMallocIdx = StackMallocSizeClass(localStackSize);
    CHECK_FATAL(stackMallocIdx <= kMaxAsanStackMallocSizeClass, "Too large stackMallocIdx");
  }
  doStackMalloc = (!HasNonEmptyInlineAsm) && (!HasReturnsTwiceCall) && doStackMalloc;

  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  MIRSymbol *allocaValue = createAllocaForLayout(insBefore, mirBuilder, L);

  for (size_t i = 0; i < stackVariableDesc.size(); i++) {
    ASanStackVariableDescription desc = stackVariableDesc.at(i);
    if (desc.Symbol != nullptr) {
      MIRSymbol *localVar = desc.Symbol;
      BinaryNode *addExpr =
          mirBuilder->CreateExprBinary(OP_add, *IntptrTy,
                                       mirBuilder->CreateExprTypeCvt(OP_cvt, IntptrTy->GetPrimType(), PTY_u64,
                                                                     *mirBuilder->CreateDread(*allocaValue, PTY_a64)),
                                       mirBuilder->CreateIntConst(desc.Offset, IntptrTy->GetPrimType()));
      // change the variable to a pointer with name asan_<Var_name>
      MIRType *localVarPtr = GlobalTables::GetTypeTable().GetOrCreatePointerType(desc.Symbol->GetTyIdx());
      MIRSymbol *newLocalVar = getOrCreateSymbol(mirBuilder, localVarPtr->GetTypeIndex(), "asan_" + localVar->GetName(),
                                                 kStVar, kScAuto, mirFunction, kScopeLocal);
      newLocalVar->SetSrcPosition(localVar->GetSrcPosition());
      // initialize the field of the Var by dassign
      DassignNode *dassignNode = mirBuilder->CreateStmtDassign(newLocalVar->GetStIdx(), 0, addExpr);
      dassignNode->InsertAfterThis(*insBefore);
      // replace the Var being referenced
      replaceAllUsesWith(localVar, newLocalVar);
      /* The stack Var could be a function call parameter
      When the stack Var is initialized by the calling convention, its shadow memory can be
      initialized already, or it has value so that we need to assign the source value to its field
      NOTE: this must be done after replaceAllUsesWith; otherwise, the initialization could be falsely
      replaced*/
      if (isFuncCallArg(localVar)) {
        // // dread asan_<Var> PTY_a64
        // BaseNode *asanAddrExpr = mirBuilder->CreateDread(*newLocalVar, PTY_a64);
        // // dread <Var> <type>
        // MIRType *argVarType = localVar->GetType();
        // BaseNode *argVarValue = mirBuilder->CreateDread(*localVar, argVarType->GetPrimType());
        // // TODO: need check->since we assign the whole value of an argument, the field should be 0
        // IassignNode *iassignNode = mirBuilder->CreateStmtIassign(*localVarPtr, 0, asanAddrExpr, argVarValue);
        // iassignNode->InsertAfterThis(*insBefore);
        // // here we call memcpy to parse the argument
        // the function should be the same as the upper code
        MapleVector<BaseNode *> args(module->GetMPAllocator().Adapter());
        args.emplace_back(mirBuilder->CreateDread(*newLocalVar, PTY_a64));
        args.emplace_back(mirBuilder->CreateAddrof(*localVar, PTY_u64));
        args.emplace_back(mirBuilder->GetConstUInt64(localVar->GetType()->GetSize()));
        IntrinsiccallNode *intrinsiccallNode = mirBuilder->CreateStmtIntrinsicCall(INTRN_C_memcpy, args);
        intrinsiccallNode->InsertAfterThis(*insBefore);
      }
    }
    if (desc.AllocaInst != nullptr) {
      BinaryNode *addExpr =
          mirBuilder->CreateExprBinary(OP_add, *IntptrTy,
                                       mirBuilder->CreateExprTypeCvt(OP_cvt, IntptrTy->GetPrimType(), PTY_u64,
                                                                     *mirBuilder->CreateDread(*allocaValue, PTY_a64)),
                                       mirBuilder->CreateIntConst(desc.Offset, IntptrTy->GetPrimType()));
      if (desc.AllocaInst->GetOpCode() == OP_regassign || desc.AllocaInst->GetOpCode() == OP_dassign) {
        UnaryStmtNode *assignNode = dynamic_cast<UnaryStmtNode *>(desc.AllocaInst);
        assignNode->SetRHS(addExpr);
      }
    }
    insBefore = insBefore->GetNext()->GetPrev();
  }

  // The left-most redzone has enough space for at least 4 pointers.
  // Write the Magic value to redzone[0].
  BaseNode *basePlus0 = mirBuilder->CreateDread(*allocaValue, PTY_a64);
  IassignNode *basePlus0Store = mirBuilder->CreateStmtIassign(
      *IntptrPtrTy, 0, basePlus0, mirBuilder->CreateIntConst(kCurrentStackFrameMagic, IntptrTy->GetPrimType()));
  basePlus0Store->InsertAfterThis(*insBefore);
  // Write the frame description constant to redzone[1]
  BaseNode *basePlus1 = mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetPrimType(PTY_u64),
                                                     mirBuilder->CreateDread(*allocaValue, PTY_a64),
                                                     mirBuilder->CreateIntConst(ASan.LongSize / 8, PTY_u64));

  ConststrNode *description = module->CurFuncCodeMemPool()->New<ConststrNode>(
      PTY_a64, GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(descriptionString));

  IassignNode *basePlus1Store = mirBuilder->CreateStmtIassign(*IntptrPtrTy, 0, basePlus1, description);
  basePlus1Store->InsertAfterThis(*insBefore);
  // Write the PC to redzone[2]
  BaseNode *basePlus2 = mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetPrimType(PTY_u64),
                                                     mirBuilder->CreateDread(*allocaValue, PTY_a64),
                                                     mirBuilder->CreateIntConst(2 * ASan.LongSize / 8, PTY_u64));
  AddroffuncNode *addroffuncNode = mirBuilder->CreateExprAddroffunc(mirFunction->GetPuidx());
  addroffuncNode->SetPrimType(PTY_a64);
  IassignNode *basePlus2Store = mirBuilder->CreateStmtIassign(*IntptrPtrTy, 0, basePlus2, addroffuncNode);
  basePlus2Store->InsertAfterThis(*insBefore);
  const auto &shadowAfterScope = GetShadowBytesAfterScope(stackVariableDesc, L);

#ifdef ENABLERBTREE
  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.emplace_back(mirBuilder->CreateDread(*allocaValue, PTY_a64));
  args.emplace_back(mirBuilder->CreateIntConst(shadowAfterScope.size() * L.Granularity, ASan.IntPtrPrim));
  auto callNode = mirBuilder->CreateStmtCall(ASan.AsanRBTStackInsert->GetPuidx(), args);
  callNode->InsertAfterThis(*insBefore);

  // Dig holes in redzone for variables
  for (auto const &desc : stackVariableDesc) {
    // LogInfo::MapleLogger() << "digging hole for " << desc.Name << "\n";
    MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
    auto redzonePtr =
        mirBuilder->CreateExprBinary(OP_add, *ASan.IntPtrTy, mirBuilder->CreateDread(*allocaValue, PTY_a64),
                                     mirBuilder->CreateIntConst(desc.Offset, ASan.IntPtrPrim));
    args.emplace_back(redzonePtr);
    args.emplace_back(mirBuilder->CreateIntConst(desc.Size, ASan.IntPtrPrim));
    auto callNode = mirBuilder->CreateStmtCall(ASan.AsanRBTStackDelete->GetPuidx(), args);
    callNode->InsertAfterThis(*insBefore);
  }

  // (Un)poison the stack before all ret instructions.
  for (StmtNode *ret : RetVec) {
    MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.emplace_back(mirBuilder->CreateDread(*allocaValue, PTY_a64));
    args.emplace_back(mirBuilder->CreateIntConst(L.FrameSize, ASan.IntPtrPrim));
    auto callNode = mirBuilder->CreateStmtCall(ASan.AsanRBTStackDelete->GetPuidx(), args);
    callNode->InsertAfterThis(*ret);
  }
#else
  // Get the value of shadow memory
  MIRSymbol *shadowBase = getOrCreateSymbol(mirBuilder, IntptrTy->GetTypeIndex(), "asan_shadowBase", kStVar, kScAuto,
                                            mirFunction, kScopeLocal);
  DassignNode *dassignNode = mirBuilder->CreateStmtDassign(
      *shadowBase, 0, ASan.memToShadow(mirBuilder->CreateDread(*allocaValue, PTY_a64), *mirBuilder));
  dassignNode->InsertAfterThis(*insBefore);
  copyToShadow(shadowAfterScope, shadowAfterScope, mirBuilder,
               mirBuilder->CreateDread(*shadowBase, shadowBase->GetType()->GetPrimType()), insBefore);

  std::vector<uint8_t> shadowClean(shadowAfterScope.size(), 0);
  std::vector<uint8_t> shadowAfterReturn;

  // (Un)poison the stack before all ret instructions.
  for (StmtNode *ret : RetVec) {
    // Mark the current frame as retired.
    IassignNode *retiredNode = mirBuilder->CreateStmtIassign(
        *IntptrPtrTy, 0, basePlus0, mirBuilder->CreateIntConst(kRetiredStackFrameMagic, IntptrTy->GetPrimType()));
    retiredNode->InsertAfterThis(*ret);
    if (doStackMalloc) {
      copyToShadow(shadowAfterScope, shadowClean, mirBuilder,
                   mirBuilder->CreateDread(*shadowBase, shadowBase->GetType()->GetPrimType()), ret);
    }
  }
#endif
  // We are done. Remove the old unused alloca instructions.
  for (ASanStackVariableDescription svd : stackVariableDesc) {
    if (svd.Symbol != nullptr) {
      svd.Symbol->SetIsDeleted();
    }
  }
}

BaseNode *FunctionStackPoisoner::GetTransformedNode(MIRSymbol *oldVar, MIRSymbol *newVar, BaseNode *baseNode) {
  BaseNode *retNode = nullptr;
  if (baseNode->GetOpCode() == OP_addrof) {
    AddrofNode *addrofNode = dynamic_cast<AddrofNode *>(baseNode);
    MIRSymbol *mirSymbol = mirFunction->GetLocalOrGlobalSymbol(addrofNode->GetStIdx());
    if (mirSymbol->GetStIdx() == oldVar->GetStIdx()) {
      retNode = module->GetMIRBuilder()->CreateDread(*newVar, PTY_a64);
      return retNode;
    }
  } else if (baseNode->GetOpCode() == OP_dassign) {
    DassignNode *dassignNode = dynamic_cast<DassignNode *>(baseNode);
    MIRSymbol *mirSymbol = mirFunction->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
    if (mirSymbol->GetStIdx() == oldVar->GetStIdx()) {
      BaseNode *newRHS = GetTransformedNode(oldVar, newVar, dassignNode->GetRHS());
      StmtNode *newStmtNode =
          module->GetMIRBuilder()->CreateStmtIassign(*newVar->GetType(), dassignNode->GetFieldID(),
                                                     module->GetMIRBuilder()->CreateDread(*newVar, PTY_a64), newRHS);
      retNode = newStmtNode;
      return retNode;
    }
  } else if (baseNode->GetOpCode() == OP_dread) {
    DreadNode *dreadNode = dynamic_cast<DreadNode *>(baseNode);
    MIRSymbol *mirSymbol = mirFunction->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
    if (mirSymbol->GetStIdx() == oldVar->GetStIdx()) {
      IreadNode *newStmtNode = module->GetMIRBuilder()->CreateExprIread(
          *GlobalTables::GetTypeTable().GetPrimType(dreadNode->GetPrimType()), *newVar->GetType(),
          dreadNode->GetFieldID(), module->GetMIRBuilder()->CreateDread(*newVar, PTY_a64));
      retNode = newStmtNode;
      return retNode;
    }
  } else if (baseNode->GetOpCode() == OP_dassignoff) {
    DassignoffNode *dassignoffNode = dynamic_cast<DassignoffNode *>(baseNode);
    MIRSymbol *mirSymbol = mirFunction->GetLocalOrGlobalSymbol(dassignoffNode->GetStIdx());
    if (mirSymbol->GetStIdx() == oldVar->GetStIdx()) {
      BaseNode *newRHS = GetTransformedNode(oldVar, newVar, dassignoffNode->GetRHS());
      StmtNode *newStmtNode =
          module->GetMIRBuilder()->CreateStmtIassignoff(newVar->GetType()->GetPrimType(),
                                                        dassignoffNode->GetOffset(),
                                                        module->GetMIRBuilder()->CreateDread(*newVar, PTY_a64),
                                                        newRHS);
      retNode = newStmtNode;
      return retNode;
    }
  }

  // all other process must run following code to make sure every child has been visited
  for (size_t j = 0; j < baseNode->NumOpnds(); j++) {
    BaseNode *tmpNode = GetTransformedNode(oldVar, newVar, baseNode->Opnd(j));
    if (tmpNode != baseNode->Opnd(j)) {
      baseNode->SetOpnd(tmpNode, j);
    }
  }
  retNode = baseNode;
  CHECK_FATAL(retNode != nullptr, "No return node.");
  return retNode;
}

void FunctionStackPoisoner::replaceAllUsesWith(MIRSymbol *oldVar, MIRSymbol *newVar) {
  if (mirFunction->GetBody() == nullptr) {
    return;
  }
  CHECK_FATAL(oldVar->GetTyIdx() == dynamic_cast<MIRPtrType *>(newVar->GetType())->GetPointedTyIdx(),
              "Replace Var SYmbol with different PointedTyIdx");
  std::vector<std::pair<StmtNode *, StmtNode *>> toReplace;
  for (StmtNode &stmt : mirFunction->GetBody()->GetStmtNodes()) {
    BaseNode *newStmt = GetTransformedNode(oldVar, newVar, &stmt);
    if (newStmt != dynamic_cast<BaseNode *>(&stmt)) {
      StmtNode *stmt2ptr = dynamic_cast<StmtNode *>(newStmt);
      CHECK_FATAL(stmt2ptr != nullptr, "Get a stmt2 without StmtNode type");
      stmt2ptr->SetSrcPos(stmt.GetSrcPos());
      toReplace.push_back(std::pair<StmtNode *, StmtNode *>(&stmt, stmt2ptr));
    }
  }

  for (auto ss : toReplace) {
    mirFunction->GetBody()->ReplaceStmt1WithStmt2(ss.first, ss.second);
  }
}
}  // namespace maple
