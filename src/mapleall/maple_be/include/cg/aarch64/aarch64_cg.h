/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_args.h"
#include "aarch64_alignment.h"
#include "aarch64_validbit_opt.h"
#include "aarch64_reg_coalesce.h"
#include "aarch64_rce.h"

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
  GCTIBKey(MapleAllocator &allocator, uint32 rcHeader, std::vector<uint64> &patternWords)
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
  int id;
  MapleString name;
  GCTIBKey *key;
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
    return mp.New<AArch64RedundantComputeElim>(f, ssaInfo);
  }

  static const AArch64MD kMd[kMopLast];
  enum : uint8 {
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
