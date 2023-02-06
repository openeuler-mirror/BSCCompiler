//
// Created by wchenbt on 4/4/2021.
//

#ifndef MAPLE_SAN_ASAN_MODULE_H
#define MAPLE_SAN_ASAN_MODULE_H

#include "mir_module.h"
#include "asan_mapping.h"
#include "san_common.h"
#include "asan_stackvar.h"
#include "mir_function.h"

namespace maple {
  class ModuleAddressSanitizer {
  public:
    ModuleAddressSanitizer(MIRModule &module) : module(&module), Mapping(getShadowMapping()) {
      int longsize = kSizeOfPtr * 8;
      IntPtrPrim = longsize == sizeof(int32) ? PTY_i32 : PTY_i64;
      IntPtrTy = GlobalTables::GetTypeTable().GetPrimType(IntPtrPrim);
      GetGlobalSymbolUsage();
    }


    bool instrumentModule();

  private:

    void initializeCallbacks();

    void GetGlobalSymbolUsage();

    bool InstrumentGlobals(BlockNode *ctorToBeInserted);
    bool ShouldInstrumentGlobal(MIRSymbol *var);

    void InstrumentGlobalsWithMetadataArray(BlockNode *ctorToBeInserted,
                                            std::vector<MIRSymbol*> ExtendedGlobals,
                                            std::vector<MIRConst*> MetadataInitializers);

    BlockNode *CreateCtorAndInitFunctions(std::string CtorName, std::string InitName,
                               MapleVector<BaseNode *> InitArgs);

    BlockNode *CreateModuleDtor();

    size_t MinRedzoneSizeForGlobal() const {
      return std::max(32U, 1U << Mapping.Scale);
    }

    MIRModule *module;

    PrimType IntPtrPrim;
    MIRType *IntPtrTy;
    ShadowMapping Mapping;

    MIRFunction *AsanRegisterGlobals, *AsanUnregisterGlobals;

    MIRFunction *AsanCtorFunction = nullptr;
    MIRFunction *AsanDtorFunction = nullptr;

    std::map<std::basic_string<char>, std::set<BaseNode *>> symbolUsedInStmt;
    std::map<std::basic_string<char>, std::set<MIRSymbol *>> symbolUsedInInit;
  };
}


#endif //MAPLE_SAN_ASAN_MODULE_H
