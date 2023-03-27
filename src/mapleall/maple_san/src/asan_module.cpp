//
// Created by wchenbt on 4/4/2021.
//

#include "asan_module.h"
#include "mir_builder.h"
#include "asan_interfaces.h"

namespace maple {
  void ModuleAddressSanitizer::initializeCallbacks() {
    MIRBuilder *mirBuilder = module->GetMIRBuilder();

    ArgVector args(module->GetMPAllocator().Adapter());
    MIRFunction *init_func = mirBuilder->CreateFunction("__cxx_global_var_init",
                                                        *GlobalTables::GetTypeTable().GetVoid(),
                                                        args, false, true);
    MIRFunction *fini_func = mirBuilder->CreateFunction("__cxx_global_var_fini",
                                                        *GlobalTables::GetTypeTable().GetVoid(),
                                                        args, false, true);
    init_func->SetAttr(FUNCATTR_local);
    fini_func->SetAttr(FUNCATTR_local);

    module->AddFunction(init_func);
    module->AddFunction(fini_func);
    MIRType *retType = GlobalTables::GetTypeTable().GetVoid();

    // Declare functions that register/unregister globals.
    AsanRegisterGlobals = getOrInsertFunction(
            mirBuilder, kAsanRegisterGlobalsName, retType, {IntPtrTy, IntPtrTy});
    AsanUnregisterGlobals = getOrInsertFunction(
            mirBuilder, kAsanUnregisterGlobalsName, retType, {IntPtrTy, IntPtrTy});

  }

  bool ModuleAddressSanitizer::instrumentModule() {
    initializeCallbacks();
    MapleVector<BaseNode *> args(module->GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
    BlockNode *ctorToBeInserted = CreateCtorAndInitFunctions(kAsanModuleCtorName, kAsanInitName, args);

    InstrumentGlobals(ctorToBeInserted);

    appendToGlobalCtors(*module, AsanCtorFunction);
    if (AsanDtorFunction) {
      appendToGlobalDtors(*module, AsanDtorFunction);
    }
    module->SetSomeSymbolNeedForDecl(false);
    return true;
  }

  bool ModuleAddressSanitizer::InstrumentGlobals(BlockNode *ctorToBeInserted) {
    std::vector<MIRSymbol *> globalsToChange;
    for (MIRSymbol *global : GetGlobalVaribles(*module)) {
      if (ShouldInstrumentGlobal(global)) {
        globalsToChange.push_back(global);
      }
    }

    size_t n = globalsToChange.size();
    if (n == 0) {
      return false;
    }
    FieldVector fieldVector;
    FieldVector parentFileds;
    std::vector<MIRSymbol *> newGlobals(n);
    std::vector<MIRConst *> initializers(n);

    // We initialize an array of such structures and pass it to a run-time call.
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "beg", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "size", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "size_with_redzone", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "name", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "module_name", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "has_dynamic_init", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "source_location", *IntPtrTy);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "odr_indicator", *IntPtrTy);
    // Create new type for global with redzones
    MIRStructType *globalStructForInitTy = static_cast<MIRStructType *>(
            GlobalTables::GetTypeTable().GetOrCreateStructType(
                    "GlobalStruct", fieldVector, parentFileds, *module));

    for (size_t i = 0; i < n; i++) {
      static const uint64_t kMaxGlobalRedzone = 1 << 18;
      MIRSymbol *global = globalsToChange[i];
      // Compute the size of redzone
      size_t sizeInBytes = global->GetType()->GetSize();
      size_t minRedZone = MinRedzoneSizeForGlobal();
      size_t redzone = std::max(minRedZone,
                                std::min(kMaxGlobalRedzone, ((sizeInBytes / minRedZone) / 4) * minRedZone));
      size_t rightRedzoneSize = redzone;
      if (sizeInBytes % minRedZone) {
        rightRedzoneSize += minRedZone - (sizeInBytes % minRedZone);
      }
      ASSERT(((rightRedzoneSize + sizeInBytes) % minRedZone) == 0,
        "rightRedzoneSize + sizeInBytes cannot be divided by minRedZone");

      // Create new type for global with redzones
      fieldVector.clear();
      parentFileds.clear();
      MIRArrayType *rightRedZoneTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(
              *GlobalTables::GetTypeTable().GetInt8(), rightRedzoneSize);
      GlobalTables::GetTypeTable().PushIntoFieldVector(
              fieldVector, "orig", *global->GetType());
      GlobalTables::GetTypeTable().PushIntoFieldVector(
              fieldVector, "redzone", *rightRedZoneTy);
      MIRStructType *newGlobalType = static_cast<MIRStructType *>(
              GlobalTables::GetTypeTable().GetOrCreateStructType(
                      "NewGlobal_" + global->GetName(), fieldVector, parentFileds, *module));

      // Create new variable for global with redzones
      MIRSymbol *newGlobalVar = module->GetMIRBuilder()->CreateSymbol(
              newGlobalType->GetTypeIndex(), "", global->GetSKind(),
              global->GetStorageClass(), nullptr, kScopeGlobal);

      // Initialize the new global
      MIRAggConst *newGlobalConst = module->GetMemPool()->
              New<MIRAggConst>(*module, *newGlobalVar->GetType());
      // Initialize the field orig
      MIRConst *globalConst = global->GetKonst();
      MIRConst *globalConstClone;
      if (globalConst->GetKind() == kConstInt) {
        globalConstClone = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
                static_cast<MIRIntConst *>(globalConst)->GetValue(), globalConst->GetType());
      } else {
        globalConstClone = globalConst->Clone(*module->GetMemPool());
      }
      newGlobalConst->AddItem(globalConstClone, 1);
      // Initialize the field redzone
      MIRAggConst *arrayConst = module->GetMemPool()->New<MIRAggConst>(*module, *rightRedZoneTy);
      for (size_t j = 0; j < rightRedzoneSize; j++) {
        arrayConst->AddItem(GlobalTables::GetIntConstTable().GetOrCreateIntConst(
                0, *GlobalTables::GetTypeTable().GetInt8()), 0);
      }

      newGlobalConst->AddItem(arrayConst, 2);
      // Set the initialized value to
      newGlobalVar->SetKonst(newGlobalConst);
      // Make the new created one the same as the old global variable
      newGlobalVar->SetAttrs(global->GetAttrs());
      newGlobalVar->SetNameStrIdx(global->GetName());
      // Set source location
      newGlobalVar->SetSrcPosition(global->GetSrcPosition());

       // replace global variable field Id
      for (MIRSymbol *mirSymbol: symbolUsedInInit[newGlobalVar->GetName()]) {
        MIRAddrofConst *mirAddrofConst = dynamic_cast<MIRAddrofConst *>(mirSymbol->GetKonst());
        MIRAddrofConst *newAddrofConst = module->GetMemPool()->New<MIRAddrofConst>(
                mirAddrofConst->GetSymbolIndex(), 1, mirAddrofConst->GetType());
        mirSymbol->SetKonst(newAddrofConst);
      }
      // replace statement field Id
      for (BaseNode *stmtNode: symbolUsedInStmt[newGlobalVar->GetName()]) {
        switch (stmtNode->GetOpCode()) {
          case OP_dassign: {
            DassignNode *dassignNode = dynamic_cast<DassignNode *>(stmtNode);
            dassignNode->SetStIdx(newGlobalVar->GetStIdx());
            dassignNode->SetFieldID(1 + dassignNode->GetFieldID());
            break;
          }
          case OP_dread:
          case OP_addrof: {
            AddrofNode *addrofNode = dynamic_cast<AddrofNode *>(stmtNode);
            addrofNode->SetStIdx(newGlobalVar->GetStIdx());
            addrofNode->SetFieldID(1 + addrofNode->GetFieldID());
            break;
          }
          case OP_callassigned: {
            CallNode *callNode = dynamic_cast<CallNode *>(stmtNode);
            CallReturnVector &callRet = callNode->GetReturnVec();
            for (size_t j = 0; j < callRet.size(); j++) {
              StIdx idx = callRet[j].first;
              RegFieldPair regFieldPair = callRet[j].second;
              if (!regFieldPair.IsReg()) {
                if (idx == global->GetStIdx()) {
                  callRet[j].first = newGlobalVar->GetStIdx();
                  callRet[j].second.SetFieldID(1 + callRet[j].second.GetFieldID());
                }
              }
            }
            break;
          }
          default: {
          }
        }
      }
      global->SetIsDeleted();
      newGlobalVar->ResetIsDeleted();
      // Create a new variable and construct its initial value
      MIRAggConst *initializer = module->GetMemPool()->New<MIRAggConst>(*module, *globalStructForInitTy);

      // begin
      MIRAddrofConst *beginConst = createAddrofConst(*module, newGlobalVar, IntPtrPrim);
      initializer->AddItem(beginConst, 1);
      // size
      MIRIntConst *sizeInBytesConst = GlobalTables::GetIntConstTable().
              GetOrCreateIntConst(sizeInBytes, *IntPtrTy);
      initializer->AddItem(sizeInBytesConst, 2);
      // size with redzone
      MIRIntConst *sizeWithRedzoneConst = GlobalTables::GetIntConstTable().
              GetOrCreateIntConst(sizeInBytes + rightRedzoneSize, *IntPtrTy);
      initializer->AddItem(sizeWithRedzoneConst, 3);
      // variable name
      MIRStrConst *nameConst = createStringConst(*module, newGlobalVar->GetName(), PTY_a64);
      initializer->AddItem(nameConst, 4);
      // module name
      MIRStrConst *moduleNameConst = createStringConst(*module, module->GetFileName(), PTY_a64);
      initializer->AddItem(moduleNameConst, 5);
      // isDynInit
      MIRIntConst *isDynInit = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *IntPtrTy);
      initializer->AddItem(isDynInit, 6);
      // Set source location
      MIRConst *sourceLocConst = createSourceLocConst(*module, newGlobalVar, IntPtrPrim);
      initializer->AddItem(sourceLocConst, 7);
      // Set OdrIndicator
      MIRConst *odrIndicator = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *IntPtrTy);
      initializer->AddItem(odrIndicator, 8);
      // Set the value of initializer
      LogInfo::MapleLogger() << "NEW GLOBAL: " << newGlobalVar->GetName() << "\n";
      newGlobals[i] = newGlobalVar;
      initializers[i] = initializer;
    }
    InstrumentGlobalsWithMetadataArray(ctorToBeInserted, newGlobals, initializers);
    return false;
  }

  bool ModuleAddressSanitizer::ShouldInstrumentGlobal(MIRSymbol *var) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(var->GetTyIdx());
    if (type == nullptr) {
      return false;
    }
    if (!isTypeSized(type)) {
      return false;
    }
    if (var->GetValue().konst == nullptr) {
      return false;
    }
    if (type->GetAlign() > MinRedzoneSizeForGlobal()) {
      return false;
    }
    return true;
  }

  void ModuleAddressSanitizer::InstrumentGlobalsWithMetadataArray(
          BlockNode *ctorToBeInserted,
          const std::vector<MIRSymbol *> ExtendedGlobals,
          std::vector<MIRConst *> MetadataInitializers) {
    assert(ExtendedGlobals.size() == MetadataInitializers.size());
    unsigned N = ExtendedGlobals.size();
    assert(N > 0);
    MIRArrayType *arrayOfGlobalStructTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(
            MetadataInitializers[0]->GetType(), N);
    MIRAggConst *allGlobalsConst = module->GetMemPool()->New<MIRAggConst>(*module, *arrayOfGlobalStructTy);
    for (MIRConst *meta: MetadataInitializers) {
      allGlobalsConst->PushBack(meta);
    }

    MIRSymbol *allGlobalsVar = module->GetMIRBuilder()->CreateSymbol(
            arrayOfGlobalStructTy->GetTypeIndex(), "allGlobals", kStConst, kScFstatic, nullptr, kScopeGlobal);
    allGlobalsVar->SetKonst(allGlobalsConst);
    MapleVector<BaseNode *> registerGlobal(module->GetMPAllocator().Adapter());
    AddrofNode *addrofNode = module->GetMIRBuilder()->CreateAddrof(*allGlobalsVar, IntPtrPrim);
    ConstvalNode *constvalNode = module->GetMIRBuilder()->CreateIntConst(N, IntPtrPrim);
    registerGlobal.emplace_back(addrofNode);
    registerGlobal.emplace_back(constvalNode);
    CallNode *registerCallNode = module->GetMIRBuilder()->CreateStmtCall(
            AsanRegisterGlobals->GetPuidx(), registerGlobal);
    ctorToBeInserted->InsertBefore(ctorToBeInserted->GetLast(), registerCallNode);
    BlockNode *dtorTobeInserted = CreateModuleDtor();
    // We also need to unregister globals at the end, e.g., when a shared library
    // gets closed.
    CallNode *unRegisterCallNode = module->GetMIRBuilder()->CreateStmtCall(
            AsanUnregisterGlobals->GetPuidx(), registerGlobal);
    dtorTobeInserted->InsertBefore(dtorTobeInserted->GetLast(), unRegisterCallNode);
  }

  BlockNode *ModuleAddressSanitizer::CreateCtorAndInitFunctions(
          const std::string CtorName, const std::string InitName, const MapleVector<BaseNode *> InitArgs) {
    MIRBuilder *mirBuilder = module->GetMIRBuilder();
    ArgVector args(module->GetMPAllocator().Adapter());
    AsanCtorFunction = mirBuilder->CreateFunction(CtorName, *GlobalTables::GetTypeTable().GetVoid(), args);
    module->AddFunction(AsanCtorFunction);
    AsanCtorFunction->SetAttr(FUNCATTR_local);
    BlockNode *asanCtorBlock = AsanCtorFunction->GetBody();
    StmtNode *retNode = mirBuilder->CreateStmtReturn(nullptr);
    asanCtorBlock->AddStatement(retNode);

    MIRFunction *initFunction = getOrInsertFunction(mirBuilder, InitName.c_str(),
                                                    GlobalTables::GetTypeTable().GetVoid(), {});
    CallNode *callInitNode = mirBuilder->CreateStmtCall(initFunction->GetPuidx(), InitArgs);

    asanCtorBlock->InsertBefore(retNode, callInitNode);
    return asanCtorBlock;
  }

  BlockNode *ModuleAddressSanitizer::CreateModuleDtor() {
    MIRBuilder *mirBuilder = module->GetMIRBuilder();
    ArgVector args(module->GetMPAllocator().Adapter());
    AsanDtorFunction = mirBuilder->CreateFunction(kAsanModuleDtorName,
                                                  *GlobalTables::GetTypeTable().GetVoid(), args);
    module->AddFunction(AsanDtorFunction);
    AsanDtorFunction->SetAttr(FUNCATTR_local);
    BlockNode *asanDtorBlock = AsanDtorFunction->GetBody();
    StmtNode *retNode = mirBuilder->CreateStmtReturn(nullptr);
    asanDtorBlock->AddStatement(retNode);

    return asanDtorBlock;
  }
  
  void ModuleAddressSanitizer::GetGlobalSymbolUsage() {
    // Replace all old global users with new global
    for (MIRFunction *func : module->GetFunctionList()) {
      if (func == nullptr || func->GetBody() == nullptr) {
        continue;
      }
      std::stack<BaseNode *> baseNodeStack;
      StmtNodes &stmtNodes = func->GetBody()->GetStmtNodes();
      for (StmtNode &stmt : stmtNodes) {
        baseNodeStack.push(&stmt);
      }

      while (!baseNodeStack.empty()) {
        BaseNode *baseNode = baseNodeStack.top();
        baseNodeStack.pop();
        switch (baseNode->GetOpCode()) {
          case OP_dassign: {
            DassignNode *dassignNode = dynamic_cast<DassignNode *>(baseNode);
            MIRSymbol *mirSymbol = func->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
            if (mirSymbol->IsGlobal()) {
              if (symbolUsedInStmt.count(mirSymbol->GetName()) == 0) {
                symbolUsedInStmt[mirSymbol->GetName()] = {};
              }
              symbolUsedInStmt[mirSymbol->GetName()].insert(dassignNode);
            }
            break;
          }
          case OP_dread:
          case OP_addrof: {
            AddrofNode *addrofNode = dynamic_cast<AddrofNode *>(baseNode);
            MIRSymbol *mirSymbol = func->GetLocalOrGlobalSymbol(addrofNode->GetStIdx());
            if (mirSymbol->IsGlobal()) {
              if (symbolUsedInStmt.count(mirSymbol->GetName()) == 0) {
                symbolUsedInStmt[mirSymbol->GetName()] = {};
              }
              symbolUsedInStmt[mirSymbol->GetName()].insert(addrofNode);
            }
            break;
          }
          case OP_callassigned: {
            CallNode *callNode = dynamic_cast<CallNode *>(baseNode);
            CallReturnVector &callRet = callNode->GetReturnVec();
            for (size_t i = 0; i < callRet.size(); i++) {
              StIdx idx = callRet[i].first;
              RegFieldPair regFieldPair = callRet[i].second;
              if (!regFieldPair.IsReg()) {
                MIRSymbol *mirSymbol = func->GetLocalOrGlobalSymbol(idx);
                if (mirSymbol->IsGlobal()) {
                  if (symbolUsedInStmt.count(mirSymbol->GetName()) == 0) {
                    symbolUsedInStmt[mirSymbol->GetName()] = {};
                  }
                  symbolUsedInStmt[mirSymbol->GetName()].insert(callNode);
                }
              }
            }
            break;
          }
          default:
            break;
        }
        for (size_t j = 0; j < baseNode->NumOpnds(); j++) {
          baseNodeStack.push(baseNode->Opnd(j));
        }
      }
    }
    for (MIRSymbol *mirSymbol: GetGlobalVaribles(*module)) {
      if (mirSymbol->GetKonst()) {
        MIRConst *mirConst = mirSymbol->GetKonst();
        if (mirConst->GetKind() == kConstAddrof) {
          MIRAddrofConst *mirAddrofConst = dynamic_cast<MIRAddrofConst*>(mirConst);
          MIRSymbol *mirSymbolUsed = GlobalTables::GetGsymTable().GetSymbolFromStidx(mirAddrofConst->GetSymbolIndex().Idx());
          if (symbolUsedInInit.count(mirSymbolUsed->GetName()) == 0) {
            symbolUsedInInit[mirSymbolUsed->GetName()] = {};
          }
          symbolUsedInInit[mirSymbolUsed->GetName()].insert(mirSymbol);
        }
      }
    }
  }
}

