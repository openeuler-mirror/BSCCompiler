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
#include "meexpr_use_info.h"

namespace maple {
class IRMapBuild; // circular dependency exists, no other choice

class IRMap : public AnalysisResult {
  friend IRMapBuild;
 public:
  struct IreadPairInfo {
    IreadPairInfo() {}

    void SetInfoOfIvar(MeExpr &baseArg, int64 offsetArg, size_t sizeArg) {
      base = &baseArg;
      bitOffset += offsetArg;
      byteSize = sizeArg;
    }

    IvarMeExpr *ivar = nullptr;
    MeExpr *base = nullptr;
    int64 bitOffset = 0;
    size_t byteSize = 0;
  };

  IRMap(SSATab &ssaTab, MemPool &memPool, uint32 hashTableSize)
      : AnalysisResult(&memPool),
        ssaTab(ssaTab),
        mirModule(ssaTab.GetModule()),
        irMapAlloc(&memPool),
        mapHashLength(hashTableSize),
        hashTable(mapHashLength, nullptr, irMapAlloc.Adapter()),
        verst2MeExprTable(ssaTab.GetVersionStTableSize(), nullptr, irMapAlloc.Adapter()),
        lpreTmps(irMapAlloc.Adapter()),
        vst2Decrefs(irMapAlloc.Adapter()),
        exprUseInfo(&memPool) {}

  ~IRMap() override = default;
  virtual BB *GetBB(BBId id) = 0;
  virtual BB *GetBBForLabIdx(LabelIdx lidx, PUIdx pidx = 0) = 0;
  MeExpr *HashMeExpr(MeExpr &meExpr);
  IvarMeExpr *BuildLHSIvarFromIassMeStmt(IassignMeStmt &iassignMeStmt);
  IvarMeExpr *BuildLHSIvar(MeExpr &baseAddr, PrimType primType, const TyIdx &tyIdx, FieldID fieldID);
  IvarMeExpr *BuildLHSIvar(MeExpr &baseAddr, IassignMeStmt &iassignMeStmt, FieldID fieldID);
  MeExpr *CreateAddrofMeExpr(MeExpr &expr);
  MeExpr *CreateAddroffuncMeExpr(PUIdx puIdx);
  MeExpr *CreateAddrofMeExprFromSymbol(MIRSymbol &st, PUIdx  puIdx);
  MeExpr *CreateIaddrofMeExpr(FieldID fieldId, TyIdx tyIdx, MeExpr *base);
  MeExpr *CreateIvarMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base);
  NaryMeExpr *CreateNaryMeExpr(const NaryMeExpr &nMeExpr);

  // for creating VarMeExpr
  VarMeExpr *CreateVarMeExprVersion(OriginalSt *ost);
  VarMeExpr *CreateVarMeExprVersion(const VarMeExpr &varx) {
    return CreateVarMeExprVersion(varx.GetOst());
  }
  RegMeExpr *CreateRegRefMeExpr(const MeExpr &meExpr);
  VarMeExpr *GetOrCreateZeroVersionVarMeExpr(OriginalSt &ost);
  VarMeExpr *CreateNewVar(GStrIdx strIdx, PrimType pType, bool isGlobal);
  VarMeExpr *CreateNewLocalRefVarTmp(GStrIdx strIdx, TyIdx tIdx);

  // for creating RegMeExpr
  RegMeExpr *CreateRegMeExprVersion(OriginalSt &pregOSt);
  RegMeExpr *CreateRegMeExprVersion(const RegMeExpr &regx) {
    return CreateRegMeExprVersion(*regx.GetOst());
  }

  ScalarMeExpr *CreateRegOrVarMeExprVersion(OStIdx ostIdx);
  RegMeExpr *CreateRegMeExpr(PrimType pType);
  RegMeExpr *CreateRegMeExpr(MIRType &mirType);
  RegMeExpr *CreateRegMeExpr(const MeExpr &meexpr) {
    MIRType *mirType = meexpr.GetType();
    if (mirType == nullptr || mirType->GetPrimType() == PTY_agg) {
      return CreateRegMeExpr(meexpr.GetPrimType());
    }
    if (meexpr.GetMeOp() == kMeOpIvar && mirType->GetPrimType() != PTY_ref) {
      return CreateRegMeExpr(meexpr.GetPrimType());
    }
    return CreateRegMeExpr(*mirType);
  }

  MeExpr *ReplaceMeExprExpr(MeExpr &origExpr, const MeExpr &meExpr, MeExpr &repExpr);
  bool ReplaceMeExprStmt(MeStmt &meStmt, const MeExpr &meExpr, MeExpr &repexpr);
  MeExpr *GetMeExprByVerID(uint32 verid) const {
    return verst2MeExprTable[verid];
  }

  MeExpr *GetMeExpr(size_t index) {
    ASSERT(index < verst2MeExprTable.size(), "index out of range");
    MeExpr *meExpr = verst2MeExprTable.at(index);
    if (meExpr == nullptr || !meExpr->IsScalar()) {
      return nullptr;
    }
    return meExpr;
  }

  IassignMeStmt *CreateIassignMeStmt(const TyIdx &tyIdx, IvarMeExpr &lhs, MeExpr &rhs,
      const MapleMap<OStIdx, ChiMeNode*> &clist);
  AssignMeStmt *CreateAssignMeStmt(ScalarMeExpr &lhs, MeExpr &rhs, BB &currBB);
  void InsertMeStmtBefore(BB&, MeStmt&, MeStmt&);
  MePhiNode *CreateMePhi(ScalarMeExpr &meExpr);

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

  MeExpr *CreateIntConstMeExpr(const IntVal &value, PrimType pType);
  MeExpr *CreateIntConstMeExpr(int64 value, PrimType pType);
  MeExpr *CreateConstMeExpr(PrimType pType, MIRConst &mirConst);
  MeExpr *CreateMeExprUnary(Opcode op, PrimType pType, MeExpr &expr0);
  MeExpr *CreateMeExprBinary(Opcode op, PrimType pType, MeExpr &expr0, MeExpr &expr1);
  MeExpr *CreateMeExprCompare(Opcode op, PrimType resptyp, PrimType opndptyp, MeExpr &opnd0, MeExpr &opnd1);
  MeExpr *CreateMeExprSelect(PrimType pType, MeExpr &expr0, MeExpr &expr1, MeExpr &expr2);
  MeExpr *CreateMeExprTypeCvt(PrimType pType, PrimType opndptyp, MeExpr &opnd0);
  MeExpr *CreateMeExprRetype(PrimType pType, TyIdx tyIdx, MeExpr &opnd);
  MeExpr *CreateMeExprExt(Opcode op, PrimType pType, uint32 bitsSize, MeExpr &opnd);
  UnaryMeStmt *CreateUnaryMeStmt(Opcode op, MeExpr *opnd) const;
  UnaryMeStmt *CreateUnaryMeStmt(Opcode op, MeExpr *opnd, BB *bb, const SrcPosition &src) const;
  RetMeStmt *CreateRetMeStmt(MeExpr *opnd);
  GotoMeStmt *CreateGotoMeStmt(uint32 offset, BB *bb, const SrcPosition *src = nullptr) const;
  IntrinsiccallMeStmt *CreateIntrinsicCallMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds,
                                                 TyIdx tyIdx = TyIdx());
  IntrinsiccallMeStmt *CreateIntrinsicCallAssignedMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds,
                                                         ScalarMeExpr *ret, TyIdx tyIdx = TyIdx());
  MeExpr *CreateCanonicalizedMeExpr(PrimType primType, Opcode opA,  Opcode opB,
                                    MeExpr *opndA, MeExpr *opndB, MeExpr *opndC);
  MeExpr *CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, MeExpr *opndA,
                                    Opcode opB, MeExpr *opndB, MeExpr *opndC);
  MeExpr *CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, Opcode opB, MeExpr *opndA, MeExpr *opndB,
                                    Opcode opC, MeExpr *opndC, MeExpr *opndD);
  MeExpr *FoldConstExprBinary(PrimType primType, Opcode op, ConstMeExpr &opndA, ConstMeExpr &opndB);
  MeExpr *FoldConstExprUnary(PrimType primType, Opcode op, ConstMeExpr &opnd);
  MeExpr *SimplifyBandExpr(const OpMeExpr *bandExpr);
  MeExpr *SimplifyLshrExpr(const OpMeExpr *shrExpr);
  MeExpr *SimplifyShlExpr(const OpMeExpr *shrExpr);
  MeExpr *SimplifySubExpr(const OpMeExpr *subExpr);
  MeExpr *SimplifyAddExpr(const OpMeExpr *addExpr);
  MeExpr *SimplifyMulExpr(const OpMeExpr *mulExpr);
  MeExpr *SimplifyCmpExpr(OpMeExpr *cmpExpr);
  static MeExpr *SimplifySelExpr(const OpMeExpr *selExpr);
  MeExpr *SimplifyOpMeExpr(OpMeExpr *opmeexpr);
  MeExpr *SimplifyOrMeExpr(OpMeExpr *opmeexpr);
  MeExpr *SimplifyAshrMeExpr(const OpMeExpr *opmeexpr);
  MeExpr *SimplifyXorMeExpr(OpMeExpr *opmeexpr);
  MeExpr *SimplifyDepositbits(const OpMeExpr &opmeexpr);
  MeExpr *SimplifyExtractbits(const OpMeExpr &opmeexpr);
  MeExpr *SimplifyMeExpr(MeExpr *x);
  void SimplifyCastForAssign(MeStmt *assignStmt) const;
  void SimplifyAssign(AssignMeStmt *assignStmt) const;
  MeExpr *SimplifyCast(MeExpr *expr);
  MeExpr* SimplifyIvarWithConstOffset(IvarMeExpr *ivar, bool lhsIvar);
  MeExpr *SimplifyIvarWithAddrofBase(IvarMeExpr *ivar);
  MeExpr *GetSimplifiedVarForIvarWithAddrofBase(OriginalSt &ost, IvarMeExpr &ivar);
  MeExpr *SimplifyIvarWithIaddrofBase(IvarMeExpr *ivar, bool lhsIvar);
  MeExpr *SimplifyIvar(IvarMeExpr *ivar, bool lhsIvar);
  void UpdateIncDecAttr(MeStmt &meStmt) const;
  static MIRType *GetArrayElemType(const MeExpr &opnd);
  bool DealWithIaddrofWhenGetInfoOfIvar(IreadPairInfo &info) const;
  bool GetInfoOfIvar(MeExpr &expr, IreadPairInfo &info) const;
  MeExpr *ReadContinuousMemory(const MeExpr &meExpr);
  MeExpr *OptBandWithIread(MeExpr &opnd0, MeExpr &opnd1);
  MeExpr *MergeAdjacentIread(MeExpr &opnd0, MeExpr &opnd1);
  bool GetIreadsInfo(MeExpr &opnd0, MeExpr &opnd1, IreadPairInfo &info0, IreadPairInfo &info1) const;
  MeExpr *CreateNewIvarForAdjacentIread(
      MeExpr &base0, const IvarMeExpr &ivar0, const IvarMeExpr &ivar1, PrimType ivarPTy, int64 newOffset);

  template <class T, typename... Arguments>
  T *NewInPool(Arguments&&... args) {
    return irMapAlloc.GetMemPool()->New<T>(&irMapAlloc, std::forward<Arguments>(args)...);
  }

  template <class T, typename... Arguments>
  T *New(Arguments&&... args) const {
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

  MapleVector<MeExpr*> &GetVerst2MeExprTable() {
    return verst2MeExprTable;
  }

  MeExpr *GetVerst2MeExprTableItem(uint32 i) const {
    if (i >= verst2MeExprTable.size()) {
      return nullptr;
    }
    return verst2MeExprTable[i];
  }

  void SetVerst2MeExprTableItem(size_t i, MeExpr *expr) {
    verst2MeExprTable[i] = expr;
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
  const MeExprUseInfo &GetExprUseInfo() const {
    return exprUseInfo;
  }

  MeExprUseInfo &GetExprUseInfo() {
    return exprUseInfo;
  }

 private:
  SSATab &ssaTab;
  MIRModule &mirModule;
  MapleAllocator irMapAlloc;
  int32 exprID = 0;                                // for allocating exprid_ in MeExpr
  uint32 mapHashLength;                            // size of hashTable
  MapleVector<MeExpr*> hashTable;                  // the value number hash table
  MapleVector<MeExpr*> verst2MeExprTable;          // map versionst to MeExpr.
  MapleUnorderedMap<OStIdx, RegMeExpr*> lpreTmps;  // for passing LPRE's temp usage to SPRE
  MapleUnorderedMap<VarMeExpr*, MapleSet<MeStmt*>*> vst2Decrefs;  // map versionst to decrefreset.
  MeExprUseInfo exprUseInfo;
  bool needAnotherPass = false;                    // set to true if CFG has changed
  bool dumpStmtNum = false;
  BB *curBB = nullptr;  // current maple_me::BB being visited

  bool ReplaceMeExprStmtOpnd(uint32 opndID, MeStmt &meStmt, const MeExpr &meExpr, MeExpr &repExpr);
  void PutToBucket(uint32 hashIdx, MeExpr &meExpr);
  const BB *GetFalseBrBB(const CondGotoMeStmt &condgoto);
  MeExpr *ReplaceMeExprExpr(MeExpr &origExpr, MeExpr &newExpr, size_t opndsSize, const MeExpr &meExpr, MeExpr &repExpr);
  MeExpr *SimplifyCompareSameExpr(const OpMeExpr *opmeexpr);
  bool IfMeExprIsU1Type(const MeExpr *expr) const;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_IRMAP_H
