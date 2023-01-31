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
#include "me_gvn.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <variant>
#include <vector>
#include "bb.h"
#include "cfg_primitive_types.h"
#include "dominance.h"
#include "global_tables.h"
#include "irmap.h"
#include "itab_util.h"
#include "maple_phase.h"
#include "me_cfg.h"
#include "me_dominance.h"
#include "me_function.h"
#include "me_ir.h"
#include "me_irmap.h"
#include "me_option.h"
#include "me_phase_manager.h"
#include "meexpr_use_info.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_const.h"
#include "mir_nodes.h"
#include "mir_type.h"
#include "mpl_logging.h"
#include "mpl_number.h"
#include "opcode_info.h"
#include "opcodes.h"
#include "scc.h"
#include "types_def.h"

namespace maple {
namespace {
size_t kVnNum = 0;
size_t kRankNum = 0;
size_t kVnExprNum = 1;
constexpr size_t kInvalidVnExprNum = 0;

bool kDebug = false;

#define DEBUG_LOG() \
  if (kDebug) LogInfo::MapleLogger() << "[GVN] "

constexpr size_t kPrefixAlign = 6; // size of "[GVN] "

void InitNum() {
  kVnNum = 0;
  kRankNum = 0;
  kVnExprNum = 1;
}

MeStmt *GetDefStmtOfIvar(const IvarMeExpr &ivar) {
  MeStmt *defStmt = ivar.GetDefStmt();
  if (defStmt != nullptr) {
    return defStmt;
  } else {
    const ScalarMeExpr *mu = ivar.GetUniqueMu();
    if (mu == nullptr) {
      return nullptr; // when irmap use simplified ivar as lhs, this case happen
    }
    ASSERT(mu != nullptr, "mu can be null only in iassign stmt, and its def stmt must not be null!");
    defStmt = mu->GetDefStmt();
    return defStmt;
  }
}

std::unique_ptr<MeExpr> CloneMeExpr(const MeExpr &expr, MapleAllocator *ma) {
  switch (expr.GetMeOp()) {
    case kMeOpIvar: {
      return std::make_unique<IvarMeExpr>(ma, kInvalidExprID, static_cast<const IvarMeExpr &>(expr));
    }
    case kMeOpOp: {
      return std::make_unique<OpMeExpr>(static_cast<const OpMeExpr &>(expr), kInvalidExprID);
    }
    case kMeOpNary: {
      return std::make_unique<NaryMeExpr>(ma, kInvalidExprID, static_cast<const NaryMeExpr &>(expr));
    }
    case kMeOpConst:
    case kMeOpVar:
    case kMeOpReg:
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist:
    case kMeOpGcmalloc: {
      return nullptr;
    }
    default:
      ASSERT(false, "Unknow meop, NYI!");
      return nullptr;
  }
}
// when a reachable BB in scc is first time visited, touch all stmts(phis) in it
bool NeedTouchBB(const BB &bb, const std::set<const BB *> &visited, const std::set<const BB *> &bbInScc) {
  return visited.count(&bb) == 0 && bbInScc.count(&bb) != 0;
}
}

// == Start of Vn
class CongruenceClass {
 public:
  struct MeExprCmp {
    bool operator()(const MeExpr *e1, const MeExpr *e2) const {
      return e1->GetExprID() < e2->GetExprID();
    }
  };

  CongruenceClass() : id(kVnNum++) {}

  explicit CongruenceClass(MeExpr &expr) : id(kVnNum++) {
    exprList.insert(&expr);
  }

  MeExpr *GetRepExpr() const {
    return exprList.empty() ? nullptr : *exprList.begin();
  }

  void AddExpr(MeExpr &expr) {
    DEBUG_LOG() << "Add expr <mx" << expr.GetExprID() << "> to <vn" << this->GetID() << ">\n";
    exprList.insert(&expr);
    if (kDebug) { this->Dump(kPrefixAlign); }
  }
  void RemoveExpr(MeExpr &expr) {
    DEBUG_LOG() << "Remove expr <mx" << expr.GetExprID() << "> from <vn" << this->GetID() << ">\n";
    exprList.erase(&expr);
    if (kDebug) { this->Dump(kPrefixAlign); }
  }
  size_t GetID() const {
    return id;
  }
  std::set<MeExpr *, MeExprCmp> &GetMeExprList() {
    return exprList;
  }

  std::vector<MeExpr *> &GetLeaderList() {
    return leaderList;
  }

  void AddLeader(MeExpr &leader) {
    DEBUG_LOG() << "Add Leader <mx" << leader.GetExprID() << "> to <vn" << this->GetID() << ">\n";
    leaderList.emplace_back(&leader);
    if (kDebug) { this->DumpLeader(kPrefixAlign); }
  }

  void Dump(size_t indent) const;
  void DumpLeader(size_t indent = 0) const;

 private:
  size_t id; // vn num
  std::set<MeExpr *, MeExprCmp> exprList;
  std::vector<MeExpr *> leaderList;
};

void CongruenceClass::Dump(size_t indent) const {
  std::string space(indent, ' ');
  LogInfo::MapleLogger() << space << "vn" << id << ": { ";
  for (const MeExpr *expr : exprList) {
    LogInfo::MapleLogger() << "mx" << expr->GetExprID() << (expr == *exprList.crbegin() ? "" : ", ");
  }
  LogInfo::MapleLogger() << " }\n";
}

void CongruenceClass::DumpLeader(size_t indent) const {
  std::string space(indent, ' ');
  LogInfo::MapleLogger() << space << "Leader of vn" << id << ": { ";
  for (const MeExpr *expr : leaderList) {
    LogInfo::MapleLogger() << "mx" << expr->GetExprID() << (expr == *leaderList.crbegin() ? "" : ", ");
  }
  LogInfo::MapleLogger() << " }\n";
}
// == End of Vn

// == Start of VnExpr
enum class VnKind {
  kVnPhi, // MePhiNode
  kVnIvar, // IvarMeExpr
  kVnNary, // NaryMeExpr
  kVnOp    // OpMeExpr
};

// VnExpr is used to represent MeExpr in an vn form
class VnExpr {
 public:
  explicit VnExpr(VnKind k) : kind(k), vnExprId(kInvalidVnExprNum) {}
  virtual ~VnExpr() = default;

  VnExpr(VnKind k, size_t id) : kind(k), vnExprId(id) {}

  virtual bool IsIdentical(const VnExpr &vnExpr) {
    (void)vnExpr;
    return false;
  }

  virtual VnExpr *GetIdenticalVnExpr(VnExpr &hashedVnExpr);

  virtual size_t GetHashedIndex() {
    return 0;
  }

  VnKind GetKind() const {
    return kind;
  }
  size_t GetVnExprID() const {
    return vnExprId;
  }
  void SetVnExprID(size_t id) {
    vnExprId = id;
  }
  void SetNext(VnExpr *n) {
    next = n;
  }
  VnExpr *GetNext() {
    return next;
  }
  virtual void Dump(size_t indent) const {
    std::string space(indent, ' ');
    LogInfo::MapleLogger() << space << "vx" << vnExprId << ": " << GetKindName() << " ";
  }

 protected:
  virtual std::string GetKindName() const {
    ASSERT(false, "NYI");
    return "";
  }

 private:
  VnKind kind;
  size_t vnExprId = kInvalidVnExprNum;
  VnExpr *next = nullptr;
};

class PhiVnExpr : public VnExpr {
 public:
  explicit PhiVnExpr(const BB &bb) : VnExpr(VnKind::kVnPhi), defBB(bb) {}
  virtual ~PhiVnExpr() = default;

  PhiVnExpr(size_t vnExprID, const PhiVnExpr &phiVnExpr) : VnExpr(VnKind::kVnPhi, vnExprID), defBB(phiVnExpr.defBB) {
    opnds.insert(opnds.end(), phiVnExpr.opnds.begin(), phiVnExpr.opnds.end());
  }

  void AddOpnd(const MeExpr &opnd) {
    opnds.emplace_back(&opnd);
  }

  bool IsIdentical(const VnExpr &vnExpr) override;

  size_t GetHashedIndex() override;

  const MeExpr *GetFirstOpnd() const {
    ASSERT(!opnds.empty(), "No Element in opnds when get first opnd!");
    return opnds.front();
  }

  bool HasOnlyOneOpnd() const {
    return opnds.size() == 1;
  }
  bool IsAllOpndsSame() const {
    const MeExpr *first = GetFirstOpnd();
    return std::all_of(opnds.begin() + 1, opnds.end(),
        [first](const MeExpr *opnd) {
          return opnd == first;
        });
  }
  const std::vector<const MeExpr *> &GetOpnds() const {
    return opnds;
  }

  void Dump(size_t indent) const override {
    VnExpr::Dump(indent);
    LogInfo::MapleLogger() << "{ BB" << defBB.GetBBId().GetIdx() << ", ";
    for (size_t i = 0; i < opnds.size(); ++i) {
      LogInfo::MapleLogger() << "mx" << opnds[i]->GetExprID() << (i == opnds.size() - 1 ? "" : ", ");
    }
    LogInfo::MapleLogger() << " }\n";
  }

 protected:
  std::string GetKindName() const override {
    return "VnPhi";
  }

 private:
  const BB &defBB;
  std::vector<const MeExpr *> opnds;
};

class IvarVnExpr : public VnExpr {
 public:
  explicit IvarVnExpr(const IvarMeExpr &ivar)
      : VnExpr(VnKind::kVnIvar),
        tyIdx(ivar.GetTyIdx()),
        fldID(ivar.GetFieldID()),
        offset(ivar.GetOffset()) {}

  IvarVnExpr(size_t vnExprID, const IvarVnExpr &ivarVnExpr)
      : VnExpr(VnKind::kVnIvar, vnExprID),
        tyIdx(ivarVnExpr.tyIdx),
        fldID(ivarVnExpr.fldID),
        offset(ivarVnExpr.offset),
        baseAddr(ivarVnExpr.baseAddr),
        mu(ivarVnExpr.mu),
        defStmtRank(ivarVnExpr.defStmtRank) {}

  virtual ~IvarVnExpr() = default;

  void SetBaseAddr(const CongruenceClass *baseVn) {
    baseAddr = baseVn;
  }
  void SetMu(const CongruenceClass *muVn) {
    mu = muVn;
  }

  void SetDefStmtRank(size_t r) {
    defStmtRank = r;
  }

  bool IsIdentical(const VnExpr &vnExpr) override;

  size_t GetHashedIndex() override;

  void Dump(size_t indent) const override {
    VnExpr::Dump(indent);
    LogInfo::MapleLogger() << "{ ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0, false);
    LogInfo::MapleLogger() << ", fld:" << fldID << ", offset:" << offset << ", base: vn" << baseAddr->GetID()
                           << ", mu: vn" << (mu != nullptr ? std::to_string(mu->GetID()) : "null")
                           << ", defStmtRank:" << defStmtRank << " }\n";
  }

 protected:
  std::string GetKindName() const override {
    return "VnIvar";
  }

 private:
  TyIdx tyIdx{ 0 };
  FieldID fldID = 0;
  int32 offset = 0;
  const CongruenceClass *baseAddr = nullptr;
  const CongruenceClass *mu = nullptr;
  size_t defStmtRank = 0;
};

class NaryVnExpr : public VnExpr {
 public:
  explicit NaryVnExpr(const NaryMeExpr &nary)
      : VnExpr(VnKind::kVnNary),
        op(nary.GetOp()),
        tyIdx(nary.GetTyIdx()),
        intrinsic(nary.GetIntrinsic()),
        boundCheck(nary.GetBoundCheck()) {}

  NaryVnExpr(size_t vnExprID, const NaryVnExpr &naryVnExpr)
      : VnExpr(VnKind::kVnNary, vnExprID),
        op(naryVnExpr.op),
        tyIdx(naryVnExpr.tyIdx),
        intrinsic(naryVnExpr.intrinsic),
        boundCheck(naryVnExpr.boundCheck) {
    opnds.insert(opnds.end(), naryVnExpr.opnds.begin(), naryVnExpr.opnds.end());
  }

  virtual ~NaryVnExpr() = default;

  void AddOpnd(const CongruenceClass &opnd) {
    opnds.emplace_back(&opnd);
  }

  bool IsIdentical(const VnExpr &vnExpr) override;

  size_t GetHashedIndex() override;

  void Dump(size_t indent) const override {
    VnExpr::Dump(indent);
    LogInfo::MapleLogger() << "{ ";
    LogInfo::MapleLogger() << "op:" << kOpcodeInfo.GetTableItemAt(op).name << ", type:" << tyIdx.GetIdx();
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0, false);
    LogInfo::MapleLogger() << ", intrinsic:" << GetIntrinsicName(intrinsic)
                           << ", boundCheck:" << boundCheck << ", opnds:";
    for (size_t i = 0; i < opnds.size(); ++i) {
      LogInfo::MapleLogger() << "vn" << opnds[i]->GetID() << (i == opnds.size() - 1 ? "" : ", ");
    }
    LogInfo::MapleLogger() << " }\n";
  }

 protected:
  std::string GetKindName() const override {
    return "VnNary";
  }

 private:
  Opcode op = OP_undef;
  TyIdx tyIdx { 0 };
  MIRIntrinsicID intrinsic;
  bool boundCheck = false;
  std::vector<const CongruenceClass *> opnds;
};

class OpVnExpr : public VnExpr {
 public:
  explicit OpVnExpr(const OpMeExpr &opExpr)
      : VnExpr(VnKind::kVnOp),
        op(opExpr.GetOp()),
        resType(opExpr.GetPrimType()),
        opndType(opExpr.GetOpndType()),
        bitsOffset(opExpr.GetBitsOffSet()),
        bitsSize(opExpr.GetBitsSize()),
        tyIdx(opExpr.GetTyIdx()),
        fieldID(opExpr.GetFieldID()) {}

  OpVnExpr(size_t vnExprID, const OpVnExpr &opVnExpr)
      : VnExpr(VnKind::kVnOp, vnExprID),
        op(opVnExpr.op),
        resType(opVnExpr.resType),
        opndType(opVnExpr.opndType),
        bitsOffset(opVnExpr.bitsOffset),
        bitsSize(opVnExpr.bitsSize),
        tyIdx(opVnExpr.tyIdx),
        fieldID(opVnExpr.fieldID) {
    opnds.insert(opnds.end(), opVnExpr.opnds.begin(), opVnExpr.opnds.end());
  }

  virtual ~OpVnExpr() = default;

  void AddOpnd(const CongruenceClass &opnd) {
    opnds.emplace_back(&opnd);
  }

  bool IsIdentical(const VnExpr &vnExpr) override;

  size_t GetHashedIndex() override;

  VnExpr *GetIdenticalVnExpr(VnExpr &hashedVnExpr) override;

  void Dump(size_t indent) const override {
    VnExpr::Dump(indent);
    LogInfo::MapleLogger() << "{ ";
    LogInfo::MapleLogger() << "op:" << kOpcodeInfo.GetTableItemAt(op).name << ", tyIdx:" << tyIdx.GetIdx()
                           << ", resType:" << GetPrimTypeName(resType) << ", opndType:" << GetPrimTypeName(opndType)
                           << ", bitsOffset:" << static_cast<int>(bitsOffset) << ", bitsSize:"
                           << static_cast<int>(bitsSize) << ", fld:" << fieldID << ", opnds:";
    for (size_t i = 0; i < opnds.size(); ++i) {
      LogInfo::MapleLogger() << "vn" << opnds[i]->GetID() << (i == opnds.size() - 1 ? "" : ", ");
    }
    LogInfo::MapleLogger() << " }\n";
  }

 protected:
  std::string GetKindName() const override {
    return "VnOp";
  }

 private:
  Opcode op = OP_undef;
  PrimType resType = kPtyInvalid;
  PrimType opndType = kPtyInvalid;  // from type
  uint8 bitsOffset = 0;
  uint8 bitsSize = 0;
  TyIdx tyIdx { 0 };
  FieldID fieldID = 0;
  std::vector<const CongruenceClass *> opnds;
};

size_t PhiVnExpr::GetHashedIndex() {
  size_t seed = 0;
  std::hash_combine(seed, static_cast<uint32>(GetKind()), defBB.GetBBId().GetIdx());
  for (auto *opnd : opnds) {
    std::hash_combine(seed, opnd->GetExprID());
  }
  return seed;
}

size_t IvarVnExpr::GetHashedIndex() {
  size_t seed = 0;
  std::hash_combine(seed, static_cast<uint32>(GetKind()), tyIdx.GetIdx(), fldID, offset,
                    baseAddr->GetID(), (mu != nullptr ? mu->GetID() : 0), defStmtRank);
  return seed;
}

size_t NaryVnExpr::GetHashedIndex() {
  size_t seed = 0;
  std::hash_combine(seed, op, tyIdx.GetIdx(), intrinsic, boundCheck);
  for (auto *opnd : opnds) {
    std::hash_combine(seed, opnd->GetID());
  }
  return seed;
}

size_t OpVnExpr::GetHashedIndex() {
  size_t seed = 0;
  std::hash_combine(seed, op, resType, opndType, bitsOffset, bitsSize, tyIdx.GetIdx(), fieldID);
  for (auto *opnd : opnds) {
    std::hash_combine(seed, opnd->GetID());
  }
  return seed;
}

bool PhiVnExpr::IsIdentical(const VnExpr &vnExpr) {
  if (vnExpr.GetKind() != VnKind::kVnPhi) {
    return false;
  }
  auto &phiVnExpr = static_cast<const PhiVnExpr &>(vnExpr);
  if (phiVnExpr.defBB.GetBBId() != defBB.GetBBId() || phiVnExpr.opnds.size() != opnds.size()) {
    return false;
  }

  for (size_t i = 0; i < opnds.size(); ++i) {
    if (phiVnExpr.opnds[i] != opnds[i]) {
      return false;
    }
  }

  return true;
}

bool IvarVnExpr::IsIdentical(const VnExpr &vnExpr) {
  if (vnExpr.GetKind() != VnKind::kVnIvar) {
    return false;
  }
  auto &ivarVnExpr = static_cast<const IvarVnExpr &>(vnExpr);
  return (ivarVnExpr.defStmtRank == defStmtRank && ivarVnExpr.tyIdx == tyIdx && ivarVnExpr.offset == offset &&
          ivarVnExpr.fldID == fldID && ivarVnExpr.baseAddr == baseAddr && ivarVnExpr.mu == mu);
}

bool NaryVnExpr::IsIdentical(const VnExpr &vnExpr) {
  if (vnExpr.GetKind() != VnKind::kVnNary) {
    return false;
  }
  auto &naryVnExpr = static_cast<const NaryVnExpr &>(vnExpr);
  if (op != naryVnExpr.op || tyIdx != naryVnExpr.tyIdx || intrinsic != naryVnExpr.intrinsic ||
      boundCheck != naryVnExpr.boundCheck) {
    return false;
  }
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (naryVnExpr.opnds[i] != opnds[i]) {
      return false;
    }
  }

  return true;
}

bool OpVnExpr::IsIdentical(const VnExpr &vnExpr) {
  if (vnExpr.GetKind() != VnKind::kVnOp) {
    return false;
  }
  auto &opVnExpr = static_cast<const OpVnExpr &>(vnExpr);
  if (op != opVnExpr.op || resType != opVnExpr.resType || opndType != opVnExpr.opndType ||
      bitsOffset != opVnExpr.bitsOffset || bitsSize != opVnExpr.bitsSize || tyIdx != opVnExpr.tyIdx ||
      fieldID != opVnExpr.fieldID) {
    return false;
  }
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (opVnExpr.opnds[i] != opnds[i]) {
      return false;
    }
  }

  return true;
}

VnExpr *VnExpr::GetIdenticalVnExpr(VnExpr &hashedVnExpr) {
  auto *vnExpr = &hashedVnExpr;
  while (vnExpr != nullptr) {
    if (IsIdentical(*vnExpr)) {
      return vnExpr;
    }
    vnExpr = vnExpr->GetNext();
  }
  return nullptr;
}

VnExpr *OpVnExpr::GetIdenticalVnExpr(VnExpr &hashedVnExpr) {
  if (kOpcodeInfo.NotPure(op)) {
    return nullptr;
  }
  return VnExpr::GetIdenticalVnExpr(hashedVnExpr);
}
// == End of VnExpr

// == Start of GVN
constexpr size_t kTableLength = 3001;

class GVN {
 public:
  using CheckItem = std::variant<MeStmt *, const MePhiNode *>;
  struct CheckItemCmp {
    explicit CheckItemCmp(const std::map<CheckItem, size_t> &r) : rank(r) {}
    bool operator()(const CheckItem &ci1, const CheckItem &ci2) const {
      return rank.at(ci1) < rank.at(ci2);
    }
    const std::map<CheckItem, size_t> &rank;
  };

  GVN(MapleAllocator &ma, MemPool &mp, MeFunction &func, Dominance &d, Dominance &pd)
      : alloc(ma),
        tmpMP(mp),
        f(func),
        cfg(*f.GetCfg()),
        dom(d),
        pdom(pd),
        irmap(*f.GetIRMap()),
        touch(CheckItemCmp(this->rank)),
        reachableBB(f.GetCfg()->GetAllBBs().size(), false),
        hashTable(kTableLength, nullptr),
        sccTopologicalVec(ma.Adapter()),
        origExprNum(func.GetIRMap()->GetExprID()) {}

  void Run();
 private:
  // prepare
  void GenSCC();
  void RankStmtAndPhi();
  bool IsBBReachable(const BB &bb) const;
  // mark bb reachable
  void MarkBBReachable(const BB &bb);
  // mark entryBB reachable
  void MarkEntryBBReachable();
  // mark succ of bb reachable according to last stmt of bb
  void MarkSuccBBReachable(const BB &bb);
  SCCNode<BB> *GetSCCofBB(BB *bb) const;
  // collect def-use info for SCC
  void CollectUseInfoInSCC(const SCCNode<BB> &scc);

  // Generate hashedVnExpr to hashTable
  void PushToBucket(size_t hashIdx, VnExpr *vn);
  VnExpr *HashVnExpr(VnExpr &vnExpr);
  // aux interface
  // try to get vn from expr2vn, return nullptr if not found
  CongruenceClass *GetVnOfMeExpr(const MeExpr &expr);
  // create a new vn for meexpr, and insert to expr2vn
  CongruenceClass *CreateVnForMeExpr(MeExpr &expr);
  MeExpr *SimplifyClonedExpr(MeExpr &expr);
  VnExpr *TransformExprToVnExpr(MeExpr &expr);
  // replace opnd with vn's repExpr and get vn from new expr
  CongruenceClass *GetOrCreateVnAfterTrans2VnExpr(MeExpr &expr);
  // VnExpr has an vnExprID after hashed
  CongruenceClass *GetOrCreateVnForHashedVnExpr(const VnExpr &hashedVnExpr);
  // create vn for expr and all its subexpr, return vn of expr
  // if it has no subexpr, the effect is the same as CreateVnForMeExpr
  // if its subexpr has vn created before, it will just use the vn and not create a new one.
  CongruenceClass *CreateVnForMeExprIteratively(MeExpr &expr);
  // try to get vn from expr2vn first, if not found, create a new one
  CongruenceClass *GetOrCreateVnForMeExpr(MeExpr &rhs);

  CongruenceClass *GetOrCreateVnForPhi(const MePhiNode &phi, const BB &bb);
  void SetVnForExpr(MeExpr &expr, CongruenceClass &vn);
  void MarkExprNeedUpdated(const MeExpr &expr);

  BB *GetFirstReachableBB(const SCCNode<BB> &scc);
  void TouchPhisStmtsInBB(BB &bb, std::set<const BB *> &visited, const std::set<const BB *> &bbInScc);
  void TouchUseSites(const MeExpr &def, const std::set<const BB *> &visitedBB);
  // generate value number for BB(s)
  void GenVnForSingleBB(BB &bb);
  void GenVnForPhi(const MePhiNode &phiItem, const BB *&currBB, const std::set<const BB *> &touchedBB);
  void GenVnForLhsValue(MeStmt &stmt, const std::set<const BB *> &touchedBB);
  void GenVnForSCCIteratively(const SCCNode<BB> &scc);

  MeExpr *GetLeaderDomExpr(MeExpr &expr, CongruenceClass &vnOfExpr, const BB &currBB);
  MeStmt *CreateLeaderAssignStmt(MeExpr &expr, CongruenceClass &vnOfExpr, BB &currBB);

  MeExpr *DoExprFRE(MeExpr &expr, MeStmt &stmt);
  void DoPhiFRE(MePhiNode &phi, MeStmt *firstStmt); // firstStmt may be nullptr
  void DoStmtFRE(MeStmt &stmt);

  void FullRedundantElimination();

  void AddToVnVector(CongruenceClass *vn);
  void DumpVnVector() const;
  void DumpVnExprs() const;
  void DumpSCC(const SCCNode<BB> &scc) const;
  void DumpSCCTopologicalVec() const;
  // dependencies
  MapleAllocator &alloc;
  MemPool &tmpMP;
  MeFunction &f;
  MeCFG &cfg;
  Dominance &dom;
  Dominance &pdom;
  IRMap &irmap;
  // structure for algorithm
  std::set<CheckItem, CheckItemCmp> touch; // worklist
  std::vector<bool> reachableBB;  // use bb id as key
  std::map<CheckItem, size_t> rank; // stmt/phi rank in rpo
  // vnx(key) and vny(pair.second) is congruent in BB(pair.first)
  std::map<size_t, std::vector<std::pair<const BB *, size_t>>> predicates;
  std::vector<CongruenceClass *> expr2Vn; // index is exprID
  std::vector<CongruenceClass *> vnExpr2Vn; // index is vnExprID
  std::vector<VnExpr *> hashTable;
  std::unordered_set<const MeExpr*> extraTouch; // used to collect CC.rep use
  MapleVector<SCCNode<BB>*> sccTopologicalVec;
  std::vector<const CongruenceClass *> vnVector; // for dump
  int32 origExprNum = 0; // used to distinguish fake expr used to simplify
};

void GVN::DumpSCC(const SCCNode<BB> &scc) const {
  auto &nodes = scc.GetNodes();
  LogInfo::MapleLogger() << "SCC " << scc.GetID() << ": { ";
  if (nodes.size() == 1) {
    LogInfo::MapleLogger() << "BB" << scc.GetNodes().front()->GetBBId().GetIdx()
                           << (scc.HasRecursion() ? "(loop)" : "(noloop)");
  } else {
    for (size_t i = 0; i < nodes.size(); ++i) {
      LogInfo::MapleLogger() << "BB" << nodes[i]->GetBBId().GetIdx() << ((i < nodes.size() - 1) ? ", " : "");
    }
  }
  LogInfo::MapleLogger() << " }\n";
}

void GVN::DumpSCCTopologicalVec() const {
  LogInfo::MapleLogger() << "-------------------\n";
  LogInfo::MapleLogger() << "---- Dump SCCs ----\n";
  LogInfo::MapleLogger() << "-------------------\n";
  for (auto *scc : sccTopologicalVec) {
    DumpSCC(*scc);
  }
  LogInfo::MapleLogger() << "-------------------\n\n";
}

void GVN::GenSCC() {
  MapleVector<BB *> &bbVec = cfg.GetAllBBs();
  size_t numOfNodes = bbVec.size();
  std::vector<BB *> allNodes;
  // copy bbVec to allNodes, but filter null BB and commonEntryBB and commonExitBB
  std::copy_if(bbVec.begin() + 2, bbVec.end(), std::inserter(allNodes, allNodes.end()), [](const BB *bb) {
    return bb != nullptr;
  });
  DEBUG_LOG() << "Building SCC\n";
  // NEEDFIX: require BB to inherit base_graph_node
  (void)BuildSCC(alloc, static_cast<uint32>(numOfNodes), allNodes, false, sccTopologicalVec);
  if (kDebug) {
    cfg.DumpToFile("gvn-scc");
    DumpSCCTopologicalVec();
  }
}

void GVN::RankStmtAndPhi() {
  DEBUG_LOG() << "Ranking stmts and phi of all BBs\n";
  for (auto *scc : sccTopologicalVec) {
    std::vector<BB*> rpo(scc->GetNodes().begin(), scc->GetNodes().end());
    const auto &bbId2RpoId = dom.GetReversePostOrderId();
    std::sort(rpo.begin(), rpo.end(), [&bbId2RpoId](BB *forward, BB *backward) {
      return bbId2RpoId[forward->GetBBId()] < bbId2RpoId[backward->GetBBId()];
    });
    for (auto *bb : rpo) {
      for (auto &phi : bb->GetMePhiList()) {
        rank[phi.second] = ++kRankNum; // start from 1
      }
      for (auto &stmt : bb->GetMeStmts()) {
        rank[&stmt] = ++kRankNum;
      }
    }
  }
}

bool GVN::IsBBReachable(const BB &bb) const {
  return reachableBB[bb.GetID()];
}

void GVN::MarkBBReachable(const BB &bb) {
  if (IsBBReachable(bb) || cfg.GetCommonEntryBB() == &bb || cfg.GetCommonExitBB() == &bb) {
    return; // mark before
  }
  // mark bb itself reachable
  DEBUG_LOG() << "Mark BB" << bb.GetBBId().GetIdx() << " reachable\n";
  reachableBB[bb.GetID()] = true;
  // mark postdominantor of bb reachable
  BB *postDominantor = static_cast<BB *>(pdom.GetDom(static_cast<uint32>(bb.GetBBId().GetIdx())));
  if (postDominantor != nullptr) {
    MarkBBReachable(*postDominantor);
  }
}

void GVN::MarkEntryBBReachable() {
  BB *commenEntryBB = cfg.GetCommonEntryBB();
  ASSERT(commenEntryBB != nullptr, "cfg must have common entry BB!");
  if (!commenEntryBB->GetSucc().empty()) {
    BB *entryBB = commenEntryBB->GetSucc(0);
    ASSERT(entryBB->GetAttributes(kBBAttrIsEntry), "succ of commonEntryBB must be entry of func!");
    MarkBBReachable(*entryBB);
  }
}


void GVN::MarkSuccBBReachable(const BB &bb) {
  switch (bb.GetKind()) {
    case kBBCondGoto: {
      const MeStmt *condMeStmt = bb.GetLastMe();
      ASSERT_NOT_NULL(condMeStmt);
      MeExpr *condMeExpr = condMeStmt->GetOpnd(0);
      CongruenceClass *vn = GetVnOfMeExpr(*condMeExpr);
      const MeExpr *repExpr = vn->GetRepExpr();
      if (repExpr->GetMeOp() == kMeOpConst) {
        const BB *destBB = (repExpr->IsIntZero()) ? bb.GetSucc(condMeStmt->GetOp() == OP_brtrue ? 0 : 1)
                                                  : bb.GetSucc(condMeStmt->GetOp() == OP_brtrue ? 1 : 0);
        MarkBBReachable(*destBB);
      } else {
        MarkBBReachable(*bb.GetSucc(0));
        MarkBBReachable(*bb.GetSucc(1));
      }
      break;
    }
    case kBBSwitch: {
      auto *switchMeStmt = static_cast<const SwitchMeStmt *>(bb.GetLastMe());
      MeExpr *condMeExpr = switchMeStmt->GetOpnd(0);
      CongruenceClass *vn = GetVnOfMeExpr(*condMeExpr);
      const MeExpr *repExpr = vn->GetRepExpr();
      if (repExpr->GetMeOp() == kMeOpConst) {
        int64 val = static_cast<const ConstMeExpr *>(repExpr)->GetSXTIntValue();
        const CaseVector &cases = switchMeStmt->GetSwitchTable();
        auto it = std::find_if(cases.begin(), cases.end(), [val](const CasePair &casePair) {
          return casePair.first == val;
        });
        LabelIdx destLableIdx = (it != cases.end()) ? it->second : switchMeStmt->GetDefaultLabel();
        MarkBBReachable(*cfg.GetLabelBBAt(destLableIdx));
      } else {
        for (auto *succ : bb.GetSucc()) {
          MarkBBReachable(*succ);
        }
      }
      break;
    }
    case kBBGoto:
    case kBBFallthru:
    case kBBAfterGosub:  // the BB that follows a gosub, as it is an entry point
    case kBBIgoto: {
      for (auto *succ : bb.GetSucc()) {
        MarkBBReachable(*succ);
      }
      break;
    }
    default:
      ASSERT(bb.GetKind() == kBBReturn || bb.GetKind() == kBBNoReturn, "BB kind is not supportable");
      break;
  }
}

// NEEDFIX: Add GetSCCNode to BB
SCCNode<BB> *GVN::GetSCCofBB(BB *bb) const {
  (void)bb;
  return nullptr;
}

void GVN::CollectUseInfoInSCC(const SCCNode<BB> &scc) {
  auto &useInfo = irmap.GetExprUseInfo();
  ASSERT(useInfo.IsInvalid(), "Require useinfo valid only for BBs in SCCNode");
  useInfo.SetState(kUseInfoOfAllExpr);
  for (BB *bb : scc.GetNodes()) {
    useInfo.CollectUseInfoInBB(bb);
  }
}

CongruenceClass *GVN::GetOrCreateVnForHashedVnExpr(const VnExpr &hashedVnExpr) {
  size_t vnExprID = hashedVnExpr.GetVnExprID();
  if (vnExprID < vnExpr2Vn.size()) {
    CongruenceClass *vn = vnExpr2Vn[vnExprID];
    ASSERT(vn != nullptr, "not set before, please check!");
    return vn;
  }
  vnExpr2Vn.resize(vnExprID + 1, nullptr);
  CongruenceClass *newVn = tmpMP.New<CongruenceClass>();
  DEBUG_LOG() << "Create new vn <vn" << newVn->GetID() << "> for vnexpr <vx" << vnExprID << ">\n";
  vnExpr2Vn[vnExprID] = newVn;
  AddToVnVector(newVn);
  return newVn;
}

CongruenceClass *GVN::GetVnOfMeExpr(const MeExpr &expr) {
  if (static_cast<size_t>(static_cast<uint32>(expr.GetExprID())) < expr2Vn.size()) {
    return expr2Vn[static_cast<uint32>(expr.GetExprID())];
  }
  return nullptr;
}

CongruenceClass *GVN::CreateVnForMeExpr(MeExpr &expr) {
  CongruenceClass *vn = tmpMP.New<CongruenceClass>(expr);
  DEBUG_LOG() << "Create new vn <vn" << vn->GetID() << "> for expr <mx" << expr.GetExprID() << ">\n";
  SetVnForExpr(expr, *vn);
  AddToVnVector(vn);
  return vn;
}

MeExpr *GVN::SimplifyClonedExpr(MeExpr &expr) {
  for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
    MeExpr *opnd = expr.GetOpnd(i);
    CongruenceClass *opndVn = GetOrCreateVnForMeExpr(*opnd);
    MeExpr *opndRepExpr = opndVn->GetRepExpr();
    // set opnd of cloned-expr as opndRepExpr
    if (expr.GetMeOp() == kMeOpIvar) {
      static_cast<IvarMeExpr &>(expr).SetBase(opndRepExpr);
    } else {
      expr.SetOpnd(i, opndRepExpr);
    }
  }
  // NEEDFIX: Simplify hashedMeExpr here
  MeExpr *hashedMeExpr = irmap.HashMeExpr(expr);
  return hashedMeExpr;
}

VnExpr *GVN::TransformExprToVnExpr(MeExpr &expr) {
  switch (expr.GetMeOp()) {
    case kMeOpIvar: {
      auto &ivar = static_cast<IvarMeExpr &>(expr);
      IvarVnExpr vnExpr(ivar);
      MeExpr *base = ivar.GetBase();
      CongruenceClass *baseVn = GetVnOfMeExpr(*base);
      vnExpr.SetBaseAddr(baseVn);
      MeStmt *stmt = GetDefStmtOfIvar(static_cast<const IvarMeExpr &>(expr));
      if (stmt == nullptr) {
        vnExpr.SetDefStmtRank(0);
      } else {
        vnExpr.SetDefStmtRank(rank[stmt]);
      }
      MeExpr *mu = ivar.GetUniqueMu();
      if (mu != nullptr) {
        CongruenceClass *muVn = GetVnOfMeExpr(*mu);
        vnExpr.SetMu(muVn);
      }

      return HashVnExpr(vnExpr);
    }
    case kMeOpOp: {
      OpVnExpr vnExpr(static_cast<OpMeExpr &>(expr));
      for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
        MeExpr *opnd = expr.GetOpnd(i);
        CongruenceClass *opndVn = GetVnOfMeExpr(*opnd);
        vnExpr.AddOpnd(*opndVn);
      }
      return HashVnExpr(vnExpr);
    }
    case kMeOpNary: {
      NaryVnExpr vnExpr(static_cast<NaryMeExpr &>(expr));
      for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
        MeExpr *opnd = expr.GetOpnd(i);
        CongruenceClass *opndVn = GetVnOfMeExpr(*opnd);
        vnExpr.AddOpnd(*opndVn);
      }
      return HashVnExpr(vnExpr);
    }
    default:
      //  has no corresponding vnExpr
      return nullptr;
  }
}

CongruenceClass *GVN::GetOrCreateVnAfterTrans2VnExpr(MeExpr &expr) {
  VnExpr *vnExpr = TransformExprToVnExpr(expr);
  CongruenceClass *vn = nullptr;
  if (vnExpr == nullptr) {
    vn = GetVnOfMeExpr(expr);
    if (vn != nullptr) {
      return vn; // vn is found in expr2Vn
    }
    // create a new vn for expr
    vn = CreateVnForMeExpr(expr);
  } else {
    vn = GetOrCreateVnForHashedVnExpr(*vnExpr);
  }
  SetVnForExpr(expr, *vn);
  return vn;
}

CongruenceClass *GVN::CreateVnForMeExprIteratively(MeExpr &expr) {
  switch (expr.GetMeOp()) {
    case kMeOpIvar:
    case kMeOpOp:
    case kMeOpNary: {
      auto clonedMeExpr = CloneMeExpr(expr, &alloc);
      MeExpr *hashedExpr = SimplifyClonedExpr(*clonedMeExpr);
      CongruenceClass *vn = GetOrCreateVnAfterTrans2VnExpr(*hashedExpr);
      SetVnForExpr(expr, *vn);
      return vn;
    }
    case kMeOpConst: // should optimize
    case kMeOpVar:
    case kMeOpReg:
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist:
    case kMeOpGcmalloc: {
      return CreateVnForMeExpr(expr);
    }
    default:
      ASSERT(false, "Unknow meop, NYI!");
      return nullptr;
  }
}

CongruenceClass *GVN::GetOrCreateVnForMeExpr(MeExpr &expr) {
  CongruenceClass *vn = nullptr;
  // for non-leaf expr, its opnd's vn may changed, so we should update its opnd with new repExpr of vn,
  // and rehash to generate a new expr
  if (expr.IsLeaf()) {
    vn = GetVnOfMeExpr(expr);
    if (vn != nullptr) {
      return vn;
    }
  }
  return CreateVnForMeExprIteratively(expr);
}


void GVN::PushToBucket(size_t hashIdx, VnExpr *vn) {
  VnExpr *head = hashTable[hashIdx];
  if (head != nullptr) {
    vn->SetNext(head);
  }
  hashTable[hashIdx] = vn;
}

// Gen HashVal and get from hashTable, if not exist, generate a new one and insert to hashTable
VnExpr *GVN::HashVnExpr(VnExpr &vnExpr) {
  size_t hashIdx = vnExpr.GetHashedIndex() % kTableLength;
  VnExpr *res = hashTable[hashIdx];
  if (res != nullptr) {
    res = vnExpr.GetIdenticalVnExpr(*res);
  }
  // vnExpr has been hashed before
  if (res != nullptr) {
    return res;
  }
  // add to hashTable
  switch (vnExpr.GetKind()) {
    case VnKind::kVnIvar: {
      res = tmpMP.New<IvarVnExpr>(kVnExprNum, static_cast<IvarVnExpr &>(vnExpr));
      break;
    }
    case VnKind::kVnPhi: {
      res = tmpMP.New<PhiVnExpr>(kVnExprNum, static_cast<PhiVnExpr &>(vnExpr));
      break;
    }
    case VnKind::kVnNary: {
      res = tmpMP.New<NaryVnExpr>(kVnExprNum, static_cast<NaryVnExpr &>(vnExpr));
      break;
    }
    case VnKind::kVnOp: {
      res = tmpMP.New<OpVnExpr>(kVnExprNum, static_cast<OpVnExpr &>(vnExpr));
      break;
    }
    default:
      ASSERT(false, "NYI");
  }
  ++kVnExprNum;
  PushToBucket(hashIdx, res);
  DEBUG_LOG() << "Create new vnExpr <vx" << res->GetVnExprID() << "> :\n";
  if (kDebug) {
    res->Dump(kPrefixAlign);
  }
  return res;
}

CongruenceClass *GVN::GetOrCreateVnForPhi(const MePhiNode &phi, const BB &bb) {
  PhiVnExpr phiVn(bb);
  for (ScalarMeExpr *opnd : phi.GetOpnds()) {
    CongruenceClass *opndVn = nullptr;
    // We should generate vn for formal/init symbol when we meet it at first time.
    if (opnd->IsZeroVersion() ||
        (opnd->IsDefByNo() && opnd->GetOst()->IsPregOst() && opnd->GetOst()->GetIndirectLev() != 0)) {
      ASSERT(opnd->GetDefBy() == kDefByNo, "Must Be");
      opndVn = GetOrCreateVnForMeExpr(*opnd);
    } else {
      opndVn = GetVnOfMeExpr(*opnd);
    }
    if (opndVn == nullptr) {
      // Here we will skip phi opnd come from backedge or unreachable edge
      continue;
    }
    const MeExpr *repExpr = opndVn->GetRepExpr();
    phiVn.AddOpnd(*repExpr);
  }
  if (phiVn.GetOpnds().empty()) {
    return nullptr;
  }
  if (phiVn.HasOnlyOneOpnd() || phiVn.IsAllOpndsSame()) {
    // if phi node has only one unique opnd, no need to hash, just return firstOpndVn
    return GetVnOfMeExpr(*phiVn.GetFirstOpnd());
  }
  VnExpr *hashedVnExpr = HashVnExpr(phiVn);
  return GetOrCreateVnForHashedVnExpr(*hashedVnExpr);
}

void GVN::SetVnForExpr(MeExpr &expr, CongruenceClass &vn) {
  if (expr2Vn.size() <= static_cast<size_t>(static_cast<uint32>(expr.GetExprID()))) {
    expr2Vn.resize(static_cast<uint32>(expr.GetExprID()) + 1, nullptr);
  }
  CongruenceClass *oldVn = expr2Vn[static_cast<uint32>(expr.GetExprID())];
  if (oldVn == &vn) {
    return;
  }
  if (oldVn != nullptr) {
    if (oldVn->GetRepExpr() == &expr && !irmap.GetExprUseInfo().IsInvalid()) {
      MeExprUseInfo &useInfo = irmap.GetExprUseInfo();
      for (auto *oldExpr : oldVn->GetMeExprList()) {
        if (!oldExpr->IsScalar()) {
          continue;
        }
        UseSitesType *useSites = useInfo.GetUseSitesOfExpr(oldExpr);
        if (useSites == nullptr) {
          continue;
        }
        for (UseItem &useItem : *useSites) {
          if (useItem.IsUseByPhi()) {
            MarkExprNeedUpdated(*oldExpr);
          }
        }
      }
    }
    oldVn->RemoveExpr(expr); // remove from oldVn
  }
  expr2Vn[static_cast<uint32>(expr.GetExprID())] = &vn;
  vn.AddExpr(expr);
}

void GVN::MarkExprNeedUpdated(const MeExpr &expr) {
  extraTouch.emplace(&expr);
}

BB *GVN::GetFirstReachableBB(const SCCNode<BB> &scc) {
  std::vector<BB*> rpo(scc.GetNodes().begin(), scc.GetNodes().end());
  const auto &bbId2RpoId = dom.GetReversePostOrderId();
  std::sort(rpo.begin(), rpo.end(), [&bbId2RpoId](BB *forward, BB *backward) {
    return bbId2RpoId[forward->GetBBId()] < bbId2RpoId[backward->GetBBId()];
  });
  auto it = std::find_if(rpo.begin(), rpo.end(), [this](const BB *bb) {
    return this->reachableBB[bb->GetID()];
  });
  if (it != rpo.end()) {
    return *it;
  }
  return nullptr;
}

void GVN::TouchPhisStmtsInBB(BB &bb, std::set<const BB *> &visited, const std::set<const BB *> &bbInScc) {
  if (!NeedTouchBB(bb, visited, bbInScc)) {
    return;
  }
  bool needTouchSucc = bb.GetMeStmts().empty();
  bool empty = bb.GetMePhiList().empty() && needTouchSucc;
  if (!empty) {
    DEBUG_LOG() << "Touch stmts/phis in BB" << bb.GetBBId().GetIdx() << "\n";
  }
  for (auto &phi : bb.GetMePhiList()) {
    if (phi.second->GetIsLive()) {
      touch.emplace(phi.second);
      needTouchSucc = false;
    }
  }
  for (auto &stmt : bb.GetMeStmts()) {
    touch.emplace(&stmt);
  }
  if (needTouchSucc) {
    DEBUG_LOG() << "BB" << bb.GetBBId().GetIdx() << " has no stmts, touch stmts/phis in its succ\n";
    for (BB *succ : bb.GetSucc()) {
      MarkBBReachable(*succ); // currBB is reachable and has no stmt, its succ must be reachable
      TouchPhisStmtsInBB(*succ, visited, bbInScc);
    }
  }
  visited.emplace(&bb);
}

void GVN::TouchUseSites(const MeExpr &def, const std::set<const BB *> &visitedBB) {
  MeExprUseInfo &useInfo = irmap.GetExprUseInfo();
  UseSitesType *useSites = useInfo.GetUseSitesOfExpr(&def);
  if (useSites == nullptr) {
    return;
  }
  DEBUG_LOG() << "Touch use sites of expr <mx" << def.GetExprID() << ">\n";
  for (UseItem &useItem : *useSites) {
    const BB *bb = useItem.GetUseBB();
    if (!IsBBReachable(*bb) || visitedBB.count(bb) == 0) {
      continue;
    }
    if (useItem.IsUseByStmt()) {
      if (useItem.GetStmt()->GetIsLive()) {
        auto res = touch.emplace(useItem.GetStmt());
        if (res.second && useItem.GetStmt()->GetLHS() != nullptr) {
          TouchUseSites(*useItem.GetStmt()->GetLHS(), visitedBB);
        }
      }
    } else {
      if (useItem.GetPhi()->GetIsLive()) {
        auto res = touch.emplace(useItem.GetPhi());
        if (res.second) {
          TouchUseSites(*useItem.GetPhi()->GetLHS(), visitedBB);
        }
      }
    }
  }
}

void GVN::GenVnForSingleBB(BB &bb) {
  DEBUG_LOG() << "++ Begin Generating GVN for Single BB" << bb.GetBBId().get() << "\n";
  if (!IsBBReachable(bb)) {
    DEBUG_LOG() << "BB" << bb.GetBBId().get() << " is not reachable, skip\n";
    return;
  }
  // 1. process phis
  for (auto &phiItem : bb.GetMePhiList()) {
    if (!phiItem.second->GetIsLive()) {
      continue;
    }
    CongruenceClass *rhsVn = GetOrCreateVnForPhi(*phiItem.second, bb);
    SetVnForExpr(*phiItem.second->GetLHS(), *rhsVn);
  }
  // 2. process stmts
  for (auto &stmt : bb.GetMeStmts()) {
    // 2.1 process opnds of stmt
    for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
      MeExpr *opnd = stmt.GetOpnd(i);
      // ensure that every opnd has a vn
      (void) GetOrCreateVnForMeExpr(*opnd);
    }

    if (stmt.GetRHS() != nullptr && stmt.IsAssign()) {
      // 2.2 process lhs of stmt
      CongruenceClass *rhsVn = GetVnOfMeExpr(*stmt.GetRHS());
      MeExpr *lhs = nullptr;
      if (stmt.GetOp() == OP_iassign) {
        lhs = static_cast<const IassignMeStmt &>(stmt).GetLHSVal();
      } else {
        lhs = stmt.GetLHS();
      }
      // implicit type conversion may cause lhs and rhs not congruent
      // Example: 1) lhs : i32 <- rhs : i64;  2) lhs : bitfield (bitsSize : 1) <- rhs : i32
      MeExpr *rhs = stmt.GetRHS();
      MIRType *rhsType = rhs->GetType();
      MIRType *lhsType = lhs->GetType();
      bool typeTrunc = false;
      if (lhs->GetPrimType() != rhs->GetPrimType()) {
        rhs = irmap.CreateMeExprTypeCvt(lhs->GetPrimType(), rhs->GetPrimType(), *rhs);
        rhsVn = GetOrCreateVnForMeExpr(*rhs);
      } else if ((lhsType != nullptr && lhsType->IsMIRBitFieldType()) ||
          (rhsType != nullptr && rhsType->IsMIRBitFieldType())) {
        typeTrunc = (lhsType != rhsType);
      }
      if (lhs->IsVolatile() || rhs->ContainsVolatile() || typeTrunc) {
        // for volatile, create an unique vn for it
        (void) GetOrCreateVnForMeExpr(*lhs);
      } else {
        SetVnForExpr(*lhs, *rhsVn);
      }
    } else if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
      // 2.3 process mustdef(return val) of stmt
      if (stmt.GetOp() == OP_asm) {
        auto *mustDefList = static_cast<AsmMeStmt &>(stmt).GetMustDefList();
        for (auto &item : *mustDefList) {
          MeExpr *lhs = item.GetLHS();
          if (lhs != nullptr) {
            (void) CreateVnForMeExpr(*lhs);
          }
        }
      } else {
        MeExpr *lhs = static_cast<AssignMeStmt &>(stmt).GetAssignedLHS();
        if (lhs != nullptr) {
          (void) CreateVnForMeExpr(*lhs);
        }
      }
    }
    // 2.4 process chilist of stmt
    if (stmt.GetChiList() != nullptr) {
      for (auto &chiItem : *stmt.GetChiList()) {
        MeExpr *lhs = chiItem.second->GetLHS();
        CongruenceClass *lhsVn = GetVnOfMeExpr(*lhs);
        if (!lhs->IsVolatile() || lhsVn == nullptr) {
          (void) CreateVnForMeExpr(*lhs);
        }
      }
    }
  }
  // 3. set succ BB reachable according to last mestmt.
  MarkSuccBBReachable(bb);

  DEBUG_LOG() << "-- End Generating GVN for Single BB" << bb.GetBBId().get() << "\n";
}

void GVN::GenVnForPhi(const MePhiNode &phiItem, const BB *&currBB, const std::set<const BB *> &touchedBB) {
  const BB *phiBB = phiItem.GetDefBB();
  if (phiBB->IsMeStmtEmpty()) {
    currBB = phiBB;
  }
  CongruenceClass *rhsVn = GetOrCreateVnForPhi(phiItem, *phiItem.GetDefBB());
  if (rhsVn == nullptr) { // rhsVn is nullptr means all opnds have no vn
    (void) GetOrCreateVnForMeExpr(*phiItem.GetLHS());
  }
  CongruenceClass *lhsVn = GetVnOfMeExpr(*phiItem.GetLHS());
  if (rhsVn != lhsVn) { // change happened
    if (rhsVn != nullptr) {
      // remove mapping : oldLhsVn <-> lhs; and add new mapping: rhsVn <-> lhs
      SetVnForExpr(*phiItem.GetLHS(), *rhsVn);
    }
    // update touch
    TouchUseSites(*phiItem.GetLHS(), touchedBB);
  }
}

void GVN::GenVnForLhsValue(MeStmt &stmt, const std::set<const BB *> &touchedBB) {
  MeExpr *lhs = nullptr;
  if (stmt.GetOp() == OP_iassign) {
    lhs = static_cast<const IassignMeStmt&>(stmt).GetLHSVal();
  } else {
    lhs = stmt.GetLHS();
  }
  if (stmt.GetRHS() != nullptr && lhs != nullptr) {
    CongruenceClass *rhsVn = GetVnOfMeExpr(*stmt.GetRHS());
    CongruenceClass *lhsVn = GetVnOfMeExpr(*lhs);
    // implicit type conversion may cause lhs and rhs not congruent
    // Example: 1) lhs : i32 <- rhs : i64;  2) lhs : bitfield (bitsSize : 1) <- rhs : i32
    MeExpr *rhs = stmt.GetRHS();
    MIRType *rhsType = rhs->GetType();
    MIRType *lhsType = lhs->GetType();
    bool typeTrunc = false;
    if (lhs->GetPrimType() != rhs->GetPrimType()) {
      rhs = irmap.CreateMeExprTypeCvt(lhs->GetPrimType(), rhs->GetPrimType(), *rhs);
      rhsVn = GetOrCreateVnForMeExpr(*rhs);
    } else if ((lhsType != nullptr && lhsType->IsMIRBitFieldType()) ||
               (rhsType != nullptr && rhsType->IsMIRBitFieldType())) {
      typeTrunc = (lhsType != rhsType);
    }
    if (lhs->IsVolatile() || rhs->ContainsVolatile() || typeTrunc) {
      (void) GetOrCreateVnForMeExpr(*lhs);
    } else if (rhsVn != lhsVn) {
      // remove mapping : oldLhsVn <-> lhs; and add new mapping: rhsVn <-> lhs
      SetVnForExpr(*lhs, *rhsVn);
      // update touch
      TouchUseSites(*lhs, touchedBB);
    }
  } else if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
    if (stmt.GetOp() == OP_asm) {
      auto *mustDefList = static_cast<AsmMeStmt&>(stmt).GetMustDefList();
      for (auto &mustDef : *mustDefList) {
        MeExpr *mustDefLHS = mustDef.GetLHS();
        if (mustDefLHS != nullptr && GetVnOfMeExpr(*mustDefLHS) == nullptr) {
          (void) GetOrCreateVnForMeExpr(*mustDefLHS);
          TouchUseSites(*mustDefLHS, touchedBB);
        }
      }
    } else {
      MeExpr *assignedLHS = static_cast<AssignMeStmt&>(stmt).GetAssignedLHS();
      if (assignedLHS != nullptr && GetVnOfMeExpr(*assignedLHS) == nullptr) {
        (void) GetOrCreateVnForMeExpr(*assignedLHS);
        TouchUseSites(*assignedLHS, touchedBB);
      }
    }
  }
  // process chilist of stmt
  if (stmt.GetChiList() != nullptr) {
    for (auto &chiItem : *stmt.GetChiList()) {
      MeExpr *chiLHS = chiItem.second->GetLHS();
      if (GetVnOfMeExpr(*chiLHS) == nullptr) {
        (void) CreateVnForMeExpr(*chiLHS);
        TouchUseSites(*chiLHS, touchedBB);
      }
    }
  }
}

void GVN::GenVnForSCCIteratively(const SCCNode<BB> &scc) {
  DEBUG_LOG() << "++ Begin Generating GVN for SCC " << scc.GetID() << "\n";
  DEBUG_LOG();
  if (kDebug) {
    DumpSCC(scc);
  }
  // 1. Init work
  // 1.2 touch stmts of first reachable BB in scc
  BB *firstBB = GetFirstReachableBB(scc);
  if (firstBB == nullptr) {
    DEBUG_LOG() << "No BB is reachable in SCC" << scc.GetID() << ", skip\n";
    return;
  }
  // when first time visit a BB in scc, touch all its stmts/phis, and record BB in touchedBB
  std::set<const BB *> touchedBB;
  // used for checking if BB is in scc.
  std::set<const BB *> bbInScc(scc.GetNodes().begin(), scc.GetNodes().end());
  TouchPhisStmtsInBB(*firstBB, touchedBB, bbInScc);
  // new useSites of changing lhsVn will be touched again
  // iterate util no new lhsVn changed and no useSites touched
  while (!touch.empty()) {
    DEBUG_LOG() << "New turn of SCC " << scc.GetID() << "\n";
    // new touched stmts/phi will be dealt with in next round
    std::vector<CheckItem> tmpTouch(touch.size());
    std::copy(touch.begin(), touch.end(), tmpTouch.begin());
    touch.clear();
    for (auto &item : tmpTouch) {
      const BB *currBB = nullptr;
      if (std::holds_alternative<const MePhiNode *>(item)) {
        // 2. process phinode
        const MePhiNode *phiItem = std::get<const MePhiNode *>(item);
        GenVnForPhi(*phiItem, currBB, touchedBB);
      } else {
        // 3. process stmt
        MeStmt *stmt = std::get<MeStmt *>(item);
        // 3.1 process opnds of stmt
        for (size_t i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
          MeExpr *opnd = stmt->GetOpnd(i);
          (void) GetOrCreateVnForMeExpr(*opnd);
        }
        // 3.2 process lhs of stmt
        GenVnForLhsValue(*stmt, touchedBB);
        const BB *stmtBB = stmt->GetBB();
        currBB = stmtBB->GetLastMe() == stmt ? stmtBB : currBB;
        if (currBB && currBB->GetKind() == kBBCondGoto &&
            (stmt->GetOpnd(0)->GetOp() == OP_eq || stmt->GetOpnd(0)->GetOp() == OP_ne)) {
          // 4. generate condition predicates
          CongruenceClass *vnx = GetVnOfMeExpr(*stmt->GetOpnd(0)->GetOpnd(0));
          CongruenceClass *vny = GetVnOfMeExpr(*stmt->GetOpnd(0)->GetOpnd(1));
          ASSERT(vnx != nullptr, "should have found CC.");
          ASSERT(vny != nullptr, "should have found CC.");
          auto vnxID = std::min(vnx->GetID(), vny->GetID());
          auto vnyID = std::max(vnx->GetID(), vny->GetID());
          predicates[vnxID].emplace_back(
              currBB->GetSucc(static_cast<size_t>(stmt->GetOpnd(0)->GetOp() == OP_eq)), vnyID);
        }
      }
      if (currBB != nullptr) {
        // 5. set succ BB reachable according to last mestmt.
        MarkSuccBBReachable(*currBB); // not all succ will be mark reachable
        for (BB *succ : currBB->GetSucc()) {
          if (IsBBReachable(*succ) && NeedTouchBB(*succ, touchedBB, bbInScc)) {
            TouchPhisStmtsInBB(*succ, touchedBB, bbInScc);
          }
        }
      }
    }
    if (!extraTouch.empty()) {
      for (auto *extra : extraTouch) {
        TouchUseSites(*extra, touchedBB);
      }
      extraTouch.clear();
    }
  }
  DEBUG_LOG() << "-- End Generating GVN for SCC " << scc.GetID() << "\n";
}

void GVN::AddToVnVector(CongruenceClass *vn) {
  if (vnVector.size() <= vn->GetID()) {
    vnVector.resize(vn->GetID() + 1, nullptr);
  }
  vnVector[vn->GetID()] = vn;
}

void GVN::DumpVnVector() const {
  LogInfo::MapleLogger() << "-------------------\n";
  LogInfo::MapleLogger() << "--- Dump All Vn ---\n";
  LogInfo::MapleLogger() << "-------------------\n";
  for (const CongruenceClass *vn : vnVector) {
    vn->Dump(0);
  }
  LogInfo::MapleLogger() << "-------------------\n\n";
}

void GVN::DumpVnExprs() const {
  LogInfo::MapleLogger() << "-------------------\n";
  LogInfo::MapleLogger() << "-- Dump VnExprs ---\n";
  LogInfo::MapleLogger() << "-------------------\n";
  constexpr size_t indent = 2;
  for (size_t i = 0; i < hashTable.size(); ++i) {
    VnExpr *vnExpr = hashTable[i];
    if (vnExpr != nullptr) {
      LogInfo::MapleLogger() << "HashTable[" << i << "]:\n";
    }
    while (vnExpr != nullptr) {
      vnExpr->Dump(indent);
      vnExpr = vnExpr->GetNext();
    }
  }
  LogInfo::MapleLogger() << "-------------------\n\n";
}

// create an assign stmt from expr to a new tmp reg
MeStmt *GVN::CreateLeaderAssignStmt(MeExpr &expr, CongruenceClass &vnOfExpr, BB &currBB) {
  // create a tmp reg as leader
  RegMeExpr *leader = irmap.CreateRegMeExpr(expr.GetPrimType());
  MeStmt *regAss = irmap.CreateAssignMeStmt(*leader, expr, currBB);
  vnOfExpr.AddLeader(*leader);
  return regAss;
}

MeExpr *GVN::GetLeaderDomExpr(MeExpr &expr, CongruenceClass &vnOfExpr, const BB &currBB) {
  auto &leaderVec = vnOfExpr.GetLeaderList();
  if (expr.GetMeOp() == kMeOpConst) {
    if (leaderVec.empty()) {
      vnOfExpr.AddLeader(expr);
    }
    if (leaderVec.front() != &expr) {
      leaderVec.back() = leaderVec.front();
      leaderVec.front() = &expr;
    }
    return &expr;
  }
  for (size_t i = 0; i < leaderVec.size(); ++i) {
    MeExpr *cand = leaderVec[i];
    if (cand->IsScalar()) {
      auto *scalar = static_cast<ScalarMeExpr *>(cand);
      BB *defBB = scalar->DefByBB();
      if (dom.Dominate(*defBB, currBB)) {
        return scalar;
      }
      // formal dominance every BB
      if (scalar->IsZeroVersion() && scalar->GetOst()->IsFormal()) {
        return scalar;
      }
    } else if (cand->GetMeOp() == kMeOpIvar) {
      ASSERT(false, "Ivar should never be a leader!");
      return nullptr;
    } else if (cand->GetMeOp() == kMeOpConst) {
      if (i != 0) {
        // make first leader as const (we may keep only one leader if it is const)
        leaderVec[i] = leaderVec[0];
        leaderVec[0] = cand;
      }
      return cand;
    }
  }
  return nullptr;
}

MeExpr *GVN::DoExprFRE(MeExpr &expr, MeStmt &stmt) {
  if (expr.IsVolatile()) {
    if (expr.GetMeOp() == kMeOpIvar) {
      std::unique_ptr<MeExpr> clonedExpr = CloneMeExpr(expr, &alloc);
      MeExpr *base = expr.GetOpnd(0);
      MeExpr *res = DoExprFRE(*base, stmt);
      if (res != base) {
        static_cast<IvarMeExpr &>(*clonedExpr).SetBase(res);
      }
      return irmap.HashMeExpr(*clonedExpr);
    }
    return &expr;
  }
  CongruenceClass *exprVn = GetVnOfMeExpr(expr);
  ASSERT(exprVn != nullptr, "Vn for expr mx%d is not set before!", expr.GetExprID());
  BB *currBB = stmt.GetBB();
  MeExpr *leader = GetLeaderDomExpr(expr, *exprVn, *currBB);
  if (leader != nullptr) {
    return leader;
  }
  std::unique_ptr<MeExpr> clonedExpr = CloneMeExpr(expr, &alloc);
  MeExpr *newExpr = &expr;
  if (clonedExpr != nullptr) {
    // FRE for opnd of expr
    for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
      MeExpr *opnd = expr.GetOpnd(i);
      MeExpr *repOpnd = DoExprFRE(*opnd, stmt);
      if (repOpnd != opnd && clonedExpr->GetMeOp() == kMeOpIvar) {
        static_cast<IvarMeExpr &>(*clonedExpr).SetBase(repOpnd);
      } else if (repOpnd != opnd) {
        clonedExpr->SetOpnd(i, repOpnd);
      }
    }
    newExpr = irmap.HashMeExpr(*clonedExpr);
  }
  auto &exprList = exprVn->GetMeExprList();
  uint32 realExprCnt = 0;
  auto &useInfo = irmap.GetExprUseInfo();
  const MeExpr *expr1 = nullptr;
  const MeExpr *expr2 = nullptr;
  const uint32 kCouple = 2;
  for (auto *recordExpr : exprList) {
    if (recordExpr->GetExprID() == kInvalidExprID || recordExpr->GetExprID() >= origExprNum) {
      continue;
    }
    if (!useInfo.GetUseSitesOfExpr(recordExpr) || useInfo.GetUseSitesOfExpr(recordExpr)->empty()) {
      continue;
    }
    realExprCnt++;
    expr1 == nullptr ? (expr1 = recordExpr) : (expr2 = recordExpr);
    if (realExprCnt > kCouple) {
      break;
    }
  }
  if (realExprCnt <= 1) {
    // expr itself has no redundancy
    return newExpr;
  }
  if (realExprCnt == kCouple && expr2) {
    ASSERT(expr1 != nullptr, "must not be null");
    // check if is in same assign
    if (expr1->IsScalar()) {
      auto *def = static_cast<const ScalarMeExpr*>(expr1)->GetDefByMeStmt();
      if (def && (def->GetRHS() == expr2 || exprList.size() == kCouple)) {
        return newExpr;
      }
    }
    if (expr2->IsScalar()) {
      auto *def = static_cast<const ScalarMeExpr*>(expr2)->GetDefByMeStmt();
      if (def && (def->GetRHS() == expr1 || exprList.size() == kCouple)) {
        return newExpr;
      }
    }
  }
  // we may create a tmp symbol for agg later
  if (newExpr->GetPrimType() == PTY_agg) {
    return newExpr;
  }
  // Create tmp reg to store the result of expr and return this reg
  MeStmt *newStmt = CreateLeaderAssignStmt(*newExpr, *exprVn, *currBB);
  currBB->InsertMeStmtBefore(&stmt, newStmt);
  return newStmt->GetLHS();
}

void GVN::DoPhiFRE(MePhiNode &phi, MeStmt *firstStmt) {
  ScalarMeExpr *lhs = phi.GetLHS();
  CongruenceClass *vn = GetVnOfMeExpr(*lhs);
  ASSERT(vn != nullptr, "Vn for phi mx%d is not set before!", lhs->GetExprID());
  auto &exprList = vn->GetMeExprList();
  uint32 realExprCnt = 0;
  auto &useInfo = irmap.GetExprUseInfo();
  for (auto *recordExpr : exprList) {
    if (recordExpr->GetExprID() == kInvalidExprID || recordExpr->GetExprID() >= origExprNum) {
      continue;
    }
    if (!useInfo.GetUseSitesOfExpr(recordExpr) || useInfo.GetUseSitesOfExpr(recordExpr)->empty()) {
      continue;
    }
    // we can not replace phi use, skip count it
    auto *useSites = useInfo.GetUseSitesOfExpr(recordExpr);
    auto it = std::find_if(useSites->begin(), useSites->end(), [this, &phi](const UseItem &use) {
      return use.IsUseByStmt() && this->dom.Dominate(*phi.GetDefBB(), *use.GetUseBB());
    });
    if (it == useSites->end()) {
      continue;
    }
    realExprCnt++;
    if (realExprCnt > 1) {
      break;
    }
  }
  if (realExprCnt <= 1) {
    // expr itself has no redundancy
    return;
  }
  BB *currBB = phi.GetDefBB();
  MeExpr *leader = GetLeaderDomExpr(*lhs, *vn, *currBB);
  if (lhs->GetPrimType() == PTY_agg ||
      (lhs->GetOst()->IsSymbolOst() && !lhs->GetOst()->GetMIRSymbol()->GetType()->IsScalarType()) ||
      lhs->GetOst()->GetIndirectLev() != 0) {
    // for symbol ost, it will create dassign stmt, and it lhs should be zero indirect level
    return;
  }
  if (leader != nullptr) {
    return;
  }
  MeStmt *assignStmt = CreateLeaderAssignStmt(*lhs, *vn, *currBB);
  // if firstStmt is nullptr, means currBB has no stmt, insert stmt at last
  // otherwise, insert assign stmt before firstStmt
  if (firstStmt == nullptr) {
    currBB->AddMeStmtLast(assignStmt);
  } else {
    currBB->InsertMeStmtBefore(firstStmt, assignStmt);
  }
}

void GVN::DoStmtFRE(MeStmt &stmt) {
  if (stmt.GetOp() == OP_asm) {
    return;
  }
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    MeExpr *opnd = stmt.GetOpnd(i);
    MeExpr *repOpnd = DoExprFRE(*opnd, stmt);
    if (repOpnd != opnd) {
      stmt.SetOpnd(i, repOpnd);
    } else {
      continue;
    }
    if (stmt.GetOp() == OP_iassign) {
      // fix lhs ivar
      static_cast<IassignMeStmt&>(stmt).SetLHSVal(irmap.BuildLHSIvarFromIassMeStmt(static_cast<IassignMeStmt&>(stmt)));
      continue;
    }
    // fix live cross:
    //   if rhs is leader, move leader reg def after origin scalar def
    if (stmt.GetOp() != OP_dassign && stmt.GetOp() != OP_regassign) {
      continue;
    }
    if (repOpnd->GetMeOp() != kMeOpReg) {
      continue;
    }
    auto *leaderDef = static_cast<ScalarMeExpr*>(repOpnd)->GetDefByMeStmt();
    if (leaderDef != stmt.GetPrev()) {
      continue;
    }
    auto *origLhs = stmt.GetLHS();
    CHECK_FATAL(origLhs != nullptr, "DoStmtFRE : check assign stmt");
    ASSERT_NOT_NULL(leaderDef);
    stmt.SetOpnd(i, leaderDef->GetRHS());
    leaderDef->SetOpnd(i, origLhs);
    leaderDef->GetBB()->RemoveMeStmt(leaderDef);
    stmt.GetBB()->InsertMeStmtAfter(&stmt, leaderDef);
  }
}

void GVN::FullRedundantElimination() {
  for (auto &scc : sccTopologicalVec) {
    for (auto *bb : scc->GetNodes()) {
      if (!IsBBReachable(*bb)) {
        continue;
      }
      // DoPhiFRE may generate new stmts at begin of BB
      MeStmt *stmt = bb->GetFirstMe();
      for (auto &phi : bb->GetMePhiList()) {
        if (phi.second->GetIsLive()) {
          DoPhiFRE(*phi.second, stmt);
        }
      }
      while (stmt) {
        MeStmt *next = stmt->GetNext();
        DoStmtFRE(*stmt);
        stmt = next;
      }
    }
  }
}

void GVN::Run() {
  DEBUG_LOG() << "+ Begin Generating GVN For Func <" << f.GetName() << ">\n\n";
  InitNum();
  // Tarjan/Nuutila algorithm to generate scc for cfg
  GenSCC();
  // traverse stmts and phi in RPO, and rank them from 1.
  RankStmtAndPhi();

  MarkEntryBBReachable();
  // collect useinfo
  auto &useInfo = irmap.GetExprUseInfo();
  if (useInfo.IsInvalid()) {
    useInfo.CollectUseInfoInFunc(&irmap, &dom, kUseInfoOfAllExpr);
  }
  // traverse scc in RPO
  for (auto *scc : sccTopologicalVec) {
    // scc has backedge
    if (scc->HasRecursion()) {
      GenVnForSCCIteratively(*scc);
    } else {
      GenVnForSingleBB(*scc->GetNodes().front());
    }
  }
  // GVN is fixed now
  DEBUG_LOG() << "- End Generating GVN For Func <" << f.GetName() << ">\n\n";
  if (kDebug) {
    DumpVnVector();
    DumpVnExprs();
  }
  FullRedundantElimination();
  useInfo.InvalidUseInfo();
}
// == End of GVN

void MEGVN::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEGVN::PhaseRun(maple::MeFunction &f) {
  if (!MeOption::gvn) {
    return false;
  }
  constexpr size_t gvnSizeLimit = 18000;
  if (f.GetCfg()->GetAllBBs().size() > gvnSizeLimit) {
    // skip big func for now
    return false;
  }
  auto *domPhase = EXEC_ANALYSIS(MEDominance, f);
  Dominance *dom = domPhase->GetDomResult();
  Dominance *pdom = domPhase->GetPdomResult();
  MapleAllocator *ma = GetPhaseAllocator();
  MemPool *tmpMP = ApplyTempMemPool();
  kDebug = DEBUGFUNC_NEWPM(f);
  GVN gvn(*ma, *tmpMP, f, *dom, *pdom);
  gvn.Run();
  // run hdse after gvn
  auto *aliasClass = FORCE_GET(MEAliasClass);
  MeHDSE hdse(f, *dom, *pdom, *f.GetIRMap(), aliasClass, DEBUGFUNC_NEWPM(f));
  hdse.hdseKeepRef = MeOption::dseKeepRef;
  hdse.DoHDSE();
  if (hdse.NeedUNClean()) {
    bool cfgChange = f.GetCfg()->UnreachCodeAnalysis(true);
    if (cfgChange) {
      FORCE_INVALID(MEDominance, f);
    }
  }
  return false;
}
} // namespace maple
