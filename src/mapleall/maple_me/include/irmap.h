/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_IRMAP_H
#define MAPLE_ME_INCLUDE_IRMAP_H
#include "bb.h"
#include "ver_symbol.h"
#include "ssa_tab.h"
#include "me_ir.h"

namespace maple {
class IRMapBuild; // circular dependency exists, no other choice

class IRMap : public AnalysisResult {
  friend IRMapBuild;
 public:
  IRMap(SSATab &ssaTab, MemPool &memPool, uint32 hashTableSize)
      : AnalysisResult(&memPool),
        ssaTab(ssaTab),
        mirModule(ssaTab.GetModule()),
        irMapAlloc(&memPool),
        mapHashLength(hashTableSize),
        hashTable(mapHashLength, nullptr, irMapAlloc.Adapter()),
        vst2MeExprTable(ssaTab.GetVersionStTableSize(), nullptr, irMapAlloc.Adapter()),
        lpreTmps(irMapAlloc.Adapter()),
        vst2Decrefs(irMapAlloc.Adapter()) {}

  virtual ~IRMap() = default;
  virtual BB *GetBB(BBId id) = 0;
  virtual BB *GetBBForLabIdx(LabelIdx lidx, PUIdx pidx = 0) = 0;
  MeExpr *HashMeExpr(MeExpr &meExpr);
  IvarMeExpr *BuildIvarFromOpMeExpr(OpMeExpr &opMeExpr);
  IvarMeExpr *BuildLHSIvarFromIassMeStmt(IassignMeStmt &iassignMeStmt);
  IvarMeExpr *BuildLHSIvar(MeExpr &baseAddr, IassignMeStmt &iassignMeStmt, FieldID fieldID);
  RegMeExpr *CreateRegRefMeExpr(const MeExpr&);
  RegMeExpr *CreateRegRefMeExpr(MIRType&);
  VarMeExpr *CreateVarMeExprVersion(const VarMeExpr&);
  MeExpr *CreateAddrofMeExpr(OStIdx ostIdx);
  MeExpr *CreateAddrofMeExpr(MeExpr&);
  MeExpr *CreateAddroffuncMeExpr(PUIdx);
  MeExpr *CreateAddrofMeExprFromSymbol(MIRSymbol& sym, PUIdx  puIdx);
  MeExpr *CreateIaddrofMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base);
  MeExpr *CreateIvarMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base);
  RegMeExpr *CreateRegMeExpr(PrimType);
  RegMeExpr *CreateRegMeExprVersion(const OriginalSt&);
  RegMeExpr *CreateRegMeExprVersion(const RegMeExpr&);
  MeExpr *ReplaceMeExprExpr(MeExpr&, const MeExpr&, MeExpr&);
  bool ReplaceMeExprStmt(MeStmt&, const MeExpr&, MeExpr&);
  MeExpr *GetMeExprByVerID(uint32 verid) const {
    return vst2MeExprTable[verid];
  }

  VarMeExpr *GetOrCreateZeroVersionVarMeExpr(const OriginalSt &ost);
  MeExpr *GetMeExpr(size_t index) {
    ASSERT(index < vst2MeExprTable.size(), "index out of range");
    MeExpr *meExpr = vst2MeExprTable.at(index);
    if (meExpr != nullptr) {
      ASSERT(meExpr->GetMeOp() == kMeOpVar || meExpr->GetMeOp() == kMeOpReg, "opcode error");
    }
    return meExpr;
  }

  VarMeExpr *CreateNewVarMeExpr(OStIdx oStIdx, PrimType pType, FieldID fieldID);
  VarMeExpr *CreateNewVarMeExpr(OriginalSt &oSt, PrimType pType, FieldID fieldID);
  VarMeExpr *CreateNewGlobalTmp(GStrIdx strIdx, PrimType pType);
  VarMeExpr *CreateNewLocalRefVarTmp(GStrIdx strIdx, TyIdx tIdx);
  DassignMeStmt *CreateDassignMeStmt(MeExpr&, MeExpr&, BB&);
  IassignMeStmt *CreateIassignMeStmt(TyIdx, IvarMeExpr&, MeExpr&, const MapleMap<OStIdx, ChiMeNode*>&);
  RegassignMeStmt *CreateRegassignMeStmt(MeExpr&, MeExpr&, BB&);
  void InsertMeStmtBefore(BB&, MeStmt&, MeStmt&);
  MePhiNode *CreateMePhi(ScalarMeExpr&);

  void DumpBB(const BB &bb) {
    int i = 0;
    for (const auto &meStmt : bb.GetMeStmts()) {
      if (GetDumpStmtNum()) {
        LogInfo::MapleLogger() << "(" << i++ << ") ";
      }
      meStmt.Dump(this);
    }
  }

  virtual void Dump() = 0;
  virtual void SetCurFunction(const BB&) {}

  MeExpr *CreateIntConstMeExpr(int64, PrimType);
  MeExpr *CreateConstMeExpr(PrimType, MIRConst&);
  MeExpr *CreateMeExprBinary(Opcode, PrimType, MeExpr&, MeExpr&);
  MeExpr *CreateMeExprCompare(Opcode, PrimType, PrimType, MeExpr&, MeExpr&);
  MeExpr *CreateMeExprSelect(PrimType, MeExpr&, MeExpr&, MeExpr&);
  MeExpr *CreateMeExprTypeCvt(PrimType, PrimType, MeExpr&);
  UnaryMeStmt *CreateUnaryMeStmt(Opcode op, MeExpr *opnd);
  UnaryMeStmt *CreateUnaryMeStmt(Opcode op, MeExpr *opnd, BB *bb, const SrcPosition *src);
  IntrinsiccallMeStmt *CreateIntrinsicCallMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds,
                                                 TyIdx tyIdx = TyIdx());
  IntrinsiccallMeStmt *CreateIntrinsicCallAssignedMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds, MeExpr *ret,
                                                         TyIdx tyIdx = TyIdx());
  MeExpr *SimplifyOpMeExpr(OpMeExpr *opmeexpr);
  MeExpr *SimplifyMeExpr(MeExpr *x);

  template <class T, typename... Arguments>
  T *NewInPool(Arguments&&... args) {
    return irMapAlloc.GetMemPool()->New<T>(&irMapAlloc, std::forward<Arguments>(args)...);
  }

  template <class T, typename... Arguments>
  T *New(Arguments&&... args) {
    return irMapAlloc.GetMemPool()->New<T>(std::forward<Arguments>(args)...);
  }

  SSATab &GetSSATab() {
    return ssaTab;
  }

  const SSATab &GetSSATab() const {
    return ssaTab;
  }

  MIRModule &GetMIRModule() {
    return mirModule;
  }

  const MIRModule &GetMIRModule() const {
    return mirModule;
  }

  MapleAllocator &GetIRMapAlloc() {
    return irMapAlloc;
  }

  int32 GetExprID() const {
    return exprID;
  }

  void SetExprID(int32 id) {
    exprID = id;
  }

  const MapleVector<MeExpr*> &GetVerst2MeExprTable() const {
    return vst2MeExprTable;
  }

  MeExpr *GetVerst2MeExprTableItem(int i) {
    return vst2MeExprTable[i];
  }

  size_t GetVerst2MeExprTableSize() const {
    return vst2MeExprTable.size();
  }

  void PushBackVerst2MeExprTable(MeExpr *item) {
    vst2MeExprTable.push_back(item);
  }

  MapleUnorderedMap<OStIdx, RegMeExpr*>::iterator GetLpreTmpsEnd() {
    return lpreTmps.end();
  }

  MapleUnorderedMap<OStIdx, RegMeExpr*>::iterator FindLpreTmpsItem(OStIdx idx) {
    return lpreTmps.find(idx);
  }

  void SetLpreTmps(OStIdx idx, RegMeExpr &expr) {
    lpreTmps[idx] = &expr;
  }

  MapleUnorderedMap<VarMeExpr*, MapleSet<MeStmt*>*> &GetVerst2DecrefsMap() {
    return vst2Decrefs;
  }

  MapleUnorderedMap<VarMeExpr*, MapleSet<MeStmt*>*>::iterator GetDecrefsEnd() {
    return vst2Decrefs.end();
  }

  MapleUnorderedMap<VarMeExpr*, MapleSet<MeStmt*>*>::iterator FindDecrefItem(VarMeExpr &var) {
    return vst2Decrefs.find(&var);
  }

  void SetDecrefs(VarMeExpr &var, MapleSet<MeStmt*> &set) {
    vst2Decrefs[&var] = &set;
  }

  void SetNeedAnotherPass(bool need) {
    needAnotherPass = need;
  }

  bool GetNeedAnotherPass() const {
    return needAnotherPass;
  }

  bool GetDumpStmtNum() const {
    return dumpStmtNum;
  }

  void SetDumpStmtNum(bool num) {
    dumpStmtNum = num;
  }

 private:
  SSATab &ssaTab;
  MIRModule &mirModule;
  MapleAllocator irMapAlloc;
  int32 exprID = 0;                                // for allocating exprid_ in MeExpr
  uint32 mapHashLength;                            // size of hashTable
  MapleVector<MeExpr*> hashTable;                  // the value number hash table
  MapleVector<MeExpr*> vst2MeExprTable;            // map versionst to MeExpr.
  MapleUnorderedMap<OStIdx, RegMeExpr*> lpreTmps;  // for passing LPRE's temp usage to SPRE
  MapleUnorderedMap<VarMeExpr*, MapleSet<MeStmt*>*> vst2Decrefs;  // map versionst to decrefreset.
  bool needAnotherPass = false;                    // set to true if CFG has changed
  bool dumpStmtNum = false;
  BB *curBB = nullptr;  // current maple_me::BB being visited

  bool ReplaceMeExprStmtOpnd(uint32, MeStmt&, const MeExpr&, MeExpr&);
  void PutToBucket(uint32, MeExpr&);
  RegMeExpr *CreateRefRegMeExpr(const MIRSymbol&);
  BB *GetFalseBrBB(const CondGotoMeStmt&);
  MeExpr *ReplaceMeExprExpr(MeExpr &origExpr, MeExpr &newExpr, size_t opndsSize, const MeExpr &meExpr, MeExpr &repExpr);
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_IRMAP_H
