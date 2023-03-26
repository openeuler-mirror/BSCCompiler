#include "asan_phases.h"
#include "asan_function.h"
#include "asan_module.h"
#include "me_cfg.h"
#include "mempool.h"


namespace maple {

void MEDoAsan::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
    aDep.AddRequired<MEDoVarCheck>();
    aDep.SetPreservedAll();
}

bool MEDoAsan::PhaseRun(maple::MeFunction &f) {
  // The reture value is said to show whether this phase modifies IR
  // The document said the return value is not used
  PreAnalysis *symbol_interesting = GET_ANALYSIS(MEDoVarCheck, f);
  if (symbol_interesting == nullptr) {
    LogInfo::MapleLogger() << "The MEDoVarCheck::PhaseRun is not called " << f.GetName() << "\n";
  }
  LogInfo::MapleLogger() << "The MEDoAsan::PhaseRun is running " << f.GetName() << "\n";
  AddressSanitizer Asan(f.GetMIRModule(), symbol_interesting);
  Asan.instrumentFunction(f);
  return true;
}

void MEDoVarCheck::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
    aDep.SetPreservedAll();
}

bool MEDoVarCheck::PhaseRun(maple::MeFunction &f) {
  LogInfo::MapleLogger() << "The MEDoVarCheck::PhaseRun is running " << f.GetName() << "\n";
  MemPool *memPool = GetPhaseMemPool();
  PreAnalysis *preAnalysis = memPool->New<PreAnalysis>(*memPool);

  MIRFunction *mirFunction = f.GetMirFunc();
  std::set<MIRSymbol*> addrOfSymList;

  std::stack<BaseNode*> baseNodeStack;
  for (StmtNode &stmt : mirFunction->GetBody()->GetStmtNodes()) {
    baseNodeStack.push(&stmt);
  }

  while (!baseNodeStack.empty()) {
    BaseNode *baseNode = baseNodeStack.top();
    baseNodeStack.pop();
    if (baseNode->GetOpCode() == OP_addrof) {
      AddrofNode *addrofNode = dynamic_cast<AddrofNode *>(baseNode);
      MIRSymbol *mirSymbol = mirFunction->GetLocalOrGlobalSymbol(addrofNode->GetStIdx());
      addrOfSymList.insert(mirSymbol);
    }
    for (size_t j = 0; j < baseNode->NumOpnds(); j++) {
      baseNodeStack.push(baseNode->Opnd(j));
    }
  }

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

    for (MIRSymbol *mirSymbol : addrOfSymList) {
      if (mirSymbol->GetStIdx() == symbol->GetStIdx()) {
        preAnalysis->usedInAddrof.push_back(symbol);
      }
    }
  }
  this->result = preAnalysis;
  LogInfo::MapleLogger() << "The MEDoVarCheck::PhaseRun ends " << f.GetName() << "\n";
  return true;
}

PreAnalysis* MEDoVarCheck::GetResult() {
    return this->result;
}

}  // namespace maple
