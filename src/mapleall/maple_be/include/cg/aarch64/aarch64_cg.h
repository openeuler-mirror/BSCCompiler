/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CG_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CG_H

#include "cg.h"
#include "aarch64_cgfunc.h"
#include "aarch64_ssa.h"
#include "aarch64_phi_elimination.h"
#include "aarch64_prop.h"
#include "aarch64_dce.h"
#include "aarch64_live.h"
#include "aarch64_reaching.h"
#include "aarch64_args.h"
#include "aarch64_alignment.h"
#include "aarch64_validbit_opt.h"
#include "aarch64_reg_coalesce.h"
#include "aarch64_rce.h"
#include "aarch64_tailcall.h"
#include "aarch64_cfgo.h"
#include "aarch64_rematerialize.h"
#include "aarch64_pgo_gen.h"
#include "aarch64_MPISel.h"
#include "aarch64_standardize.h"

namespace maplebe {
constexpr int64 kShortBRDistance = (8 * 1024);
constexpr int64 kNegativeImmLowerLimit = -4096;
constexpr int32 kIntRegTypeNum = 5;
constexpr uint32 kAlignPseudoSize = 3;
constexpr uint32 kInsnSize = 4;
constexpr uint32 kAlignMovedFlag = 31;

/* Supporting classes for GCTIB merging */
class GCTIBKey {
 public:
  GCTIBKey(MapleAllocator &allocator, uint32 rcHeader, const std::vector<uint64> &patternWords)
      : header(rcHeader), bitMapWords(allocator.Adapter()) {
    (void)bitMapWords.insert(bitMapWords.cbegin(), patternWords.cbegin(), patternWords.cend());
  }

  ~GCTIBKey() = default;

  uint32 GetHeader() const {
    return header;
  }

  const MapleVector<uint64> &GetBitmapWords() const {
    return bitMapWords;
  }

 private:
  uint32 header;
  MapleVector<uint64> bitMapWords;
};

class Hasher {
 public:
  size_t operator()(const GCTIBKey *key) const {
    CHECK_NULL_FATAL(key);
    size_t hash = key->GetHeader();
    return hash;
  }
};

class EqualFn {
 public:
  bool operator()(const GCTIBKey *firstKey, const GCTIBKey *secondKey) const {
    CHECK_NULL_FATAL(firstKey);
    CHECK_NULL_FATAL(secondKey);
    const MapleVector<uint64> &firstWords = firstKey->GetBitmapWords();
    const MapleVector<uint64> &secondWords = secondKey->GetBitmapWords();

    if ((firstKey->GetHeader() != secondKey->GetHeader()) || (firstWords.size() != secondWords.size())) {
      return false;
    }

    for (size_t i = 0; i < firstWords.size(); ++i) {
      if (firstWords[i] != secondWords[i]) {
        return false;
      }
    }
    return true;
  }
};

class GCTIBPattern {
 public:
  GCTIBPattern(GCTIBKey &patternKey, MemPool &mp) : name(&mp) {
    key = &patternKey;
    id = GetId();
    name = GCTIB_PREFIX_STR + std::string("PTN_") + std::to_string(id);
  }

  ~GCTIBPattern() = default;

  int GetId() const {
    static int createNum = 0;
    return createNum++;
  }

  std::string GetName() const {
    ASSERT(!name.empty(), "null name check!");
    return std::string(name.c_str());
  }

  void SetName(const std::string &ptnName) {
    name = ptnName;
  }

 private:
  int id = 0;
  MapleString name;
  GCTIBKey *key = nullptr;
};

/* sub Target info & implement */
class AArch64CG : public CG {
 public:
  AArch64CG(MIRModule &mod, const CGOptions &opts, const std::vector<std::string> &nameVec,
            const std::unordered_map<std::string, std::vector<std::string>> &patternMap)
      : CG(mod, opts),
        ehExclusiveNameVec(nameVec),
        cyclePatternMap(patternMap),
        keyPatternMap(allocator.Adapter()),
        symbolPatternMap(allocator.Adapter()) {}

  ~AArch64CG() override = default;

  CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool,
                       StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId) override {
    return memPool.New<AArch64CGFunc>(mod, *this, mirFunc, bec, memPool, stackMp, mallocator, funcId);
  }

  MemLayout *CreateMemLayout(MemPool &mp, BECommon &b, MIRFunction &f,
                             MapleAllocator &mallocator) const override {
    return mp.New<AArch64MemLayout>(b, f, mallocator);
  }

  RegisterInfo *CreateRegisterInfo(MemPool &mp, MapleAllocator &mallocator) const override {
    return mp.New<AArch64RegInfo>(mallocator);
  }

  void EnrollTargetPhases(MaplePhaseManager *pm) const override;

  const std::unordered_map<std::string, std::vector<std::string>> &GetCyclePatternMap() const {
    return cyclePatternMap;
  }

  void GenerateObjectMaps(BECommon &beCommon) override;

  bool IsExclusiveFunc(MIRFunction &mirFunc) override;

  void FindOrCreateRepresentiveSym(std::vector<uint64> &bitmapWords, uint32 rcHeader, const std::string &name);

  void CreateRefSymForGlobalPtn(GCTIBPattern &ptn) const;

  Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) override;

  PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) override;

  std::string FindGCTIBPatternName(const std::string &name) const override;

  LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64LiveAnalysis>(f, mp);
  }
  ReachingDefinition *CreateReachingDefinition(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64ReachingDefinition>(f, mp);
  }
  MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64MoveRegArgs>(f);
  }
  AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64AlignAnalysis>(f, mp);
  }
  CGSSAInfo *CreateCGSSAInfo(MemPool &mp, CGFunc &f, DomAnalysis &da, MemPool &tmp) const override {
    return mp.New<AArch64CGSSAInfo>(f, da, mp, tmp);
  }
  LiveIntervalAnalysis *CreateLLAnalysis(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64LiveIntervalAnalysis>(f, mp);
  }
  PhiEliminate *CreatePhiElimintor(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    return mp.New<AArch64PhiEliminate>(f, ssaInfo, mp);
  }
  CGProp *CreateCGProp(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo, LiveIntervalAnalysis &ll) const override {
    return mp.New<AArch64Prop>(mp, f, ssaInfo, ll);
  }
  CGDce *CreateCGDce(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    return mp.New<AArch64Dce>(mp, f, ssaInfo);
  }
  ValidBitOpt *CreateValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    return mp.New<AArch64ValidBitOpt>(f, ssaInfo);
  }
  RedundantComputeElim *CreateRedundantCompElim(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    return mp.New<AArch64RedundantComputeElim>(f, ssaInfo, mp);
  }
  TailCallOpt *CreateCGTailCallOpt(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64TailCallOpt>(mp, f);
  }
  CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64CFGOptimizer>(f, mp);
  }
  Rematerializer *CreateRematerializer(MemPool &mp) const override {
    return mp.New<AArch64Rematerializer>();
  }
  MPISel *CreateMPIsel(MemPool &mp, AbstractIRBuilder &aIRBuilder, CGFunc &f) const override {
    return mp.New<AArch64MPIsel>(mp, aIRBuilder, f);
  }
  Standardize *CreateStandardize(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64Standardize>(f);
  }
  /* Return the copy operand id of reg1 if it is an insn who just do copy from reg1 to reg2.
 * i. mov reg2, reg1
 * ii. add/sub reg2, reg1, 0/zero register
 * iii. mul reg2, reg1, 1
 */
  bool IsEffectiveCopy(Insn &insn) const final;
  bool IsTargetInsn(MOperator mOp) const final;
  bool IsClinitInsn(MOperator mOp) const final;
  void DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const final;
  const InsnDesc &GetTargetMd(MOperator mOp) const final {
    return kMd[mOp];
  }
  CGProfGen *CreateCGProfGen(MemPool &mp, CGFunc &f) const override {
    return mp.New<AArch64ProfGen>(f, mp);
  };

  static const InsnDesc kMd[kMopLast];
  static constexpr MOperator movBetweenOpnds[kIndex2][kIndex2][kIndex2][kIndex2] = {
    {
      { {MOP_wmovrr, MOP_undef}, {MOP_xvmovrs, MOP_undef} },
      { {MOP_undef, MOP_xmovrr}, {MOP_undef, MOP_xvmovrd} }
    },
    {
      { {MOP_xvmovsr, MOP_undef}, {MOP_xvmovs, MOP_undef} },
      { {MOP_undef, MOP_xvmovdr}, {MOP_undef, MOP_xvmovd} }
    }
  };

  static void UpdateMopOfPropedInsn(Insn &insn) {
    auto oldMop = insn.GetMachineOpcode();
    if ((oldMop < MOP_xmovrr || oldMop > MOP_xvmovrv) && oldMop != MOP_vmovuu) {
      return;
    }
    auto &leftOpnd = insn.GetOperand(kInsnFirstOpnd);
    auto &rightOpnd = insn.GetOperand(kInsnSecondOpnd);
    if (rightOpnd.IsImmediate() || leftOpnd.IsImmediate()) {
      return;
    }
    auto &leftReg = static_cast<RegOperand &>(leftOpnd);
    auto &rightReg = static_cast<RegOperand &>(rightOpnd);
    auto isLeftRegFloat = leftReg.GetRegisterType() == kRegTyFloat ? 1 : 0;
    auto isLeftReg64Bits = leftReg.GetSize() == k64BitSize ? 1 : 0;
    auto isRightRegFloat = rightReg.GetRegisterType() == kRegTyFloat ? 1 : 0;
    auto isRightReg64Bits = rightReg.GetSize() == k64BitSize ? 1 : 0;
    auto mop = movBetweenOpnds[isLeftRegFloat][isLeftReg64Bits][isRightRegFloat][isRightReg64Bits];
    if (mop == MOP_undef) {
      return;
    }
    insn.SetMOP(AArch64CG::kMd[mop]);
    insn.ClearRegSpecList();
  }

  enum RegListType: uint8 {
    kR8List,
    kR16List,
    kR32List,
    kR64List,
    kV64List
  };
  static std::array<std::array<const std::string, kAllRegNum>, kIntRegTypeNum> intRegNames;
  static std::array<const std::string, kAllRegNum> vectorRegNames;

 private:
  const std::vector<std::string> &ehExclusiveNameVec;
  const std::unordered_map<std::string, std::vector<std::string>> &cyclePatternMap;
  MapleUnorderedMap<GCTIBKey*, GCTIBPattern*, Hasher, EqualFn> keyPatternMap;
  MapleUnorderedMap<std::string, GCTIBPattern*> symbolPatternMap;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CG_H */
