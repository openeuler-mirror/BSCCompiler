/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "instrument.h"
#include "cgbb.h"

#include "mir_builder.h"
namespace maple {
std::string GetProfCntSymbolName(const std::string &funcName, PUIdx idx) {
  return funcName + "_" + std::to_string(idx) + "_counter";
}

static inline void RegisterInFuncInfo(MIRFunction &func, const MIRSymbol &counter, uint32 elemCnt, uint32 cfgHash) {
  if (elemCnt == 0) {
    return;
  }
  PUIdx idx = func.GetPuidx();
  auto funcStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(
      namemangler::kprefixProfFuncDesc + std::to_string(idx));
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(funcStrIdx);
  CHECK_FATAL(sym, "function have not generated, please check in CreateFuncInfo");
  auto *funcInfoMirConst = static_cast<MIRAggConst *>(sym->GetKonst());
  MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
  MIRIntConst *cfgHashConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(cfgHash, *u32Ty);
  MIRIntConst *eleCntMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(elemCnt, *u32Ty);
  auto *counterConst = func.GetModule()->GetMemPool()->New<MIRAddrofConst>(
      counter.GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
  funcInfoMirConst->SetItem(1, cfgHashConst, 2);
  funcInfoMirConst->SetItem(2, eleCntMirConst, 3);
  funcInfoMirConst->SetItem(3, counterConst, 4);
}

MIRSymbol *GetOrCreateFuncCounter(MIRFunction &func, uint32 elemCnt, uint32 cfgHash) {
  auto *mirModule = func.GetModule();
  std::string name = GetProfCntSymbolName(func.GetName(), func.GetPuidx());
  auto nameStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(name);
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(nameStrIdx);
  if (sym != nullptr) {
    return sym;
  }
  auto *elemType = GlobalTables::GetTypeTable().GetUInt64();
  MIRArrayType &arrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, elemCnt);
  sym = mirModule->GetMIRBuilder()->CreateGlobalDecl(name.c_str(), arrayType);
  auto *profTab = mirModule->GetMemPool()->New<MIRAggConst>(*mirModule, arrayType);
  MIRIntConst *indexConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *elemType);
  for (uint32 i = 0; i < elemCnt; ++i) {
    profTab->AddItem(indexConst, i);
  }
  sym->SetKonst(profTab);
  sym->SetStorageClass(kScFstatic);
  sym->sectionAttr = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName("mpl_counter");
  RegisterInFuncInfo(func, *sym, elemCnt, cfgHash);
  return sym;
}

template<class IRBB, class Edge>
void PGOInstrumentTemplate<IRBB, Edge>::GetInstrumentBBs(std::vector<IRBB*> &bbs, IRBB* commonEntry) const {
  std::vector<Edge*> iEdges;
  mst.GetInstrumentEdges(iEdges);
  std::unordered_set<IRBB*> visitedBBs;
  for (auto &edge : iEdges) {
    IRBB *src = edge->GetSrcBB();
    IRBB *dest = edge->GetDestBB();
    if (src->GetSuccs().size() <= 1) {
      if (src == commonEntry) {
        bbs.push_back(dest);
      } else {
        bbs.push_back(src);
      }
    } else if (!edge->IsCritical()) {
      bbs.push_back(dest);
    } else {
      if (src->GetKind() == IRBB::kBBIgoto) {
        if (visitedBBs.find(dest) == visitedBBs.end()) {
          // In this case, we have to instrument it anyway
          bbs.push_back(dest);
          (void)visitedBBs.insert(dest);
        }
      } else {
        CHECK_FATAL(false, "Unexpected case %d -> %d", src->GetId(), dest->GetId());
      }
    }
  }
}

template<typename BB>
BBUseEdge<BB> *BBUseInfo<BB>::GetOnlyUnknownOutEdges() {
  BBUseEdge<BB> *ouEdge = nullptr;
  for (auto *e : outEdges) {
    if (!e->GetStatus()) {
      CHECK_FATAL(!ouEdge, "have multiple unknown out edges");
      ouEdge = e;
    }
  }
  return ouEdge;
}

template<typename BB>
BBUseEdge<BB> *BBUseInfo<BB>::GetOnlyUnknownInEdges() {
  BBUseEdge<BB> *ouEdge = nullptr;
  for (auto *e : inEdges) {
    if (!e->GetStatus()) {
      CHECK_FATAL(!ouEdge, "have multiple unknown in edges");
      ouEdge = e;
    }
  }
  return ouEdge;
}

template class PGOInstrumentTemplate<maplebe::BB, maple::BBEdge<maplebe::BB>>;
template class PGOInstrumentTemplate<maplebe::BB, maple::BBUseEdge<maplebe::BB>>;
template class BBUseInfo<maplebe::BB>;
} /* namespace maple */
