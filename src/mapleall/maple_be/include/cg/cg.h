/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_CG_H
#define MAPLEBE_INCLUDE_CG_CG_H

/* C++ headers. */
#include <string>
/* MapleIR headers. */
#include "operand.h"
#include "insn.h"
#include "cgfunc.h"
#include "live.h"
#include "cg_option.h"
#include "opcode_info.h"
#include "global_tables.h"
#include "mir_function.h"
#include "mad.h"

namespace maplebe {
#define ADDTARGETPHASE(PhaseName, condition)    \
  if (!CGOptions::IsSkipPhase(PhaseName)) {     \
    pm->AddPhase(PhaseName, condition);         \
  }
/* subtarget opt phase -- cyclic Dependency, use Forward declaring */
class CGSSAInfo;
class PhiEliminate;
class DomAnalysis;
class CGProp;
class CGDce;
class AlignAnalysis;
class MoveRegArgs;
class MPISel;
class Standardize;
class LiveIntervalAnalysis;
class ValidBitOpt;
class CG;
class LocalOpt;
class CFGOptimizer;
class RedundantComputeElim;
class TailCallOpt;
class Rematerializer;
class CGProfGen;

class Globals {
 public:
  static Globals *GetInstance() {
    static Globals instance;
    return &instance;
  }

  ~Globals() = default;

  void SetBECommon(BECommon &bc) {
    beCommon = &bc;
  }

  BECommon *GetBECommon() {
    return beCommon;
  }

  const BECommon *GetBECommon() const {
    return beCommon;
  }

  void SetMAD(MAD &m) {
    mad = &m;
  }

  MAD *GetMAD() {
    return mad;
  }

  const MAD *GetMAD() const {
    return mad;
  }

  void SetOptimLevel(int32 opLevel) {
    optimLevel = opLevel;
  }

  int32 GetOptimLevel() const {
    return optimLevel;
  }

  void SetTarget(CG &target);
  const CG *GetTarget() const ;

 private:
  BECommon *beCommon = nullptr;
  MAD *mad = nullptr;
  int32 optimLevel = 0;
  CG *cg = nullptr;
  Globals() = default;
};

class CG {
 public:
  using GenerateFlag = uint64;

 public:
  CG(MIRModule &mod, const CGOptions &cgOptions)
      : memPool(memPoolCtrler.NewMemPool("maplecg mempool", false /* isLocalPool */)),
        allocator(memPool),
        mirModule(&mod),
        emitter(nullptr),
        labelOrderCnt(0),
        cgOption(cgOptions),
        fileGP(nullptr) {
    const std::string &internalNameLiteral = namemangler::GetInternalNameLiteral(namemangler::kJavaLangObjectStr);
    GStrIdx strIdxFromName = GlobalTables::GetStrTable().GetStrIdxFromName(internalNameLiteral);
    isLibcore = (GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdxFromName) != nullptr);
    DefineDebugTraceFunctions();
    isLmbc = (mirModule->GetFlavor() == MIRFlavor::kFlavorLmbc);
  }

  virtual ~CG();

  /* enroll all code generator phases for target machine */
  virtual void EnrollTargetPhases(MaplePhaseManager *pm) const = 0;

  void GenExtraTypeMetadata(const std::string &classListFileName, const std::string &outputBaseName);
  void GenPrimordialObjectList(const std::string &outputBaseName);
  const std::string ExtractFuncName(const std::string &str) const;

  virtual Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) = 0;
  virtual PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) = 0;

  virtual CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction&, BECommon&, MemPool&, StackMemPool&,
                               MapleAllocator&, uint32) = 0;

  bool IsExclusiveEH() const {
    return CGOptions::IsExclusiveEH();
  }

  virtual bool IsExclusiveFunc(MIRFunction &mirFunc) = 0;

  /* NOTE: Consider making be_common a field of CG. */
  virtual void GenerateObjectMaps(BECommon &beCommon) = 0;

  /* Used for GCTIB pattern merging */
  virtual std::string FindGCTIBPatternName(const std::string &name) const = 0;

  bool GenerateVerboseAsm() const {
    return cgOption.GenerateVerboseAsm();
  }

  bool GenerateVerboseCG() const {
    return cgOption.GenerateVerboseCG();
  }

  bool DoPrologueEpilogue() const {
    return cgOption.DoPrologueEpilogue();
  }

  bool DoCheckSOE() const {
    return cgOption.DoCheckSOE();
  }

  bool GenerateDebugFriendlyCode() const {
    return cgOption.GenerateDebugFriendlyCode();
  }

  int32 GetOptimizeLevel() const {
    return cgOption.GetOptimizeLevel();
  }

  bool UseFastUnwind() const {
    return true;
  }

  bool IsStackProtectorStrong() const {
    return cgOption.IsStackProtectorStrong();
  }

  bool IsStackProtectorAll() const {
    return cgOption.IsStackProtectorAll();
  }

  bool IsUnwindTables() const {
    return cgOption.IsUnwindTables();
  }

  bool InstrumentWithDebugTraceCall() const {
    return cgOption.InstrumentWithDebugTraceCall();
  }

  bool DoPatchLongBranch() const {
    return cgOption.DoPatchLongBranch() || (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0);
  }

  uint8 GetRematLevel() const {
    return CGOptions::GetRematLevel();
  }

  bool GenYieldPoint() const {
    return cgOption.GenYieldPoint();
  }

  bool GenLocalRC() const {
    return cgOption.GenLocalRC();
  }

  bool GenerateExceptionHandlingCode() const {
    return cgOption.GenerateExceptionHandlingCode();
  }

  bool DoConstFold() const {
    return cgOption.DoConstFold();
  }

  void AddStackGuardvar() const;
  void DefineDebugTraceFunctions();
  MIRModule *GetMIRModule() {
    return mirModule;
  }

  void SetEmitter(Emitter &emitterVal) {
    this->emitter = &emitterVal;
  }

  Emitter *GetEmitter() const {
    return emitter;
  }

  void IncreaseLabelOrderCnt() {
    labelOrderCnt++;
  }

  LabelIDOrder GetLabelOrderCnt() const {
    return labelOrderCnt;
  }

  const CGOptions &GetCGOptions() const {
    return cgOption;
  }

  void UpdateCGOptions(const CGOptions &newOption) {
    cgOption.SetOptionFlag(newOption.GetOptionFlag());
  }

  bool IsLibcore() const {
    return isLibcore;
  }

  bool IsLmbc() const {
    return isLmbc;
  }

  MIRSymbol *GetDebugTraceEnterFunction() {
    return dbgTraceEnter;
  }

  const MIRSymbol *GetDebugTraceEnterFunction() const {
    return dbgTraceEnter;
  }

  MIRSymbol *GetProfileFunction() {
    return dbgFuncProfile;
  }

  const MIRSymbol *GetProfileFunction() const {
    return dbgFuncProfile;
  }

  const MIRSymbol *GetDebugTraceExitFunction() const {
    return dbgTraceExit;
  }

  /* Init SubTarget Info */
  virtual MemLayout *CreateMemLayout(MemPool &mp, BECommon &b,
      MIRFunction &f, MapleAllocator &mallocator) const = 0;
  virtual RegisterInfo *CreateRegisterInfo(MemPool &mp, MapleAllocator &mallocator) const = 0;

  /* Init SubTarget phase */
  virtual LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const = 0;
  virtual ReachingDefinition *CreateReachingDefinition(MemPool &mp, CGFunc &f) const {
    return nullptr;
  };
  virtual MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const  = 0;
  virtual AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f) const = 0;
  virtual MPISel *CreateMPIsel(MemPool &mp, AbstractIRBuilder &aIRBuilder, CGFunc &f) const {
    return nullptr;
  }
  virtual Standardize *CreateStandardize(MemPool &mp, CGFunc &f) const {
    return nullptr;
  }

  /* Init SubTarget optimization */
  virtual CGSSAInfo *CreateCGSSAInfo(MemPool &mp, CGFunc &f, DomAnalysis &da, MemPool &tmp) const = 0;
  virtual LiveIntervalAnalysis *CreateLLAnalysis(MemPool &mp, CGFunc &f) const = 0;
  virtual PhiEliminate *CreatePhiElimintor(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const = 0;
  virtual CGProp *CreateCGProp(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo, LiveIntervalAnalysis &ll) const = 0;
  virtual CGDce *CreateCGDce(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const = 0;
  virtual ValidBitOpt *CreateValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const = 0;
  virtual RedundantComputeElim *CreateRedundantCompElim(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const = 0;
  virtual TailCallOpt *CreateCGTailCallOpt(MemPool &mp, CGFunc &f) const = 0;
  virtual LocalOpt *CreateLocalOpt(MemPool &mp, CGFunc &f, ReachingDefinition&) const {
    return nullptr;
  };
  virtual CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f) const {
    return nullptr;
  }
  virtual CGProfGen *CreateCGProfGen(MemPool &mp, CGFunc &f) const {
    return nullptr;
  }

  virtual Rematerializer *CreateRematerializer(MemPool &mp) const {
    return nullptr;
  }

  /* Object map generation helper */
  std::vector<int64> GetReferenceOffsets64(const BECommon &beCommon, MIRStructType &structType) const;

  void SetGP(MIRSymbol *sym) {
    fileGP = sym;
  }

  const MIRSymbol *GetGP() const {
    return fileGP;
  }
  MIRSymbol *GetGP() {
    return fileGP;
  }

  static bool IsInFuncWrapLabels(MIRFunction *func) {
    return funcWrapLabels.find(func) != funcWrapLabels.end();
  }

  static void SetFuncWrapLabels(MIRFunction *func, const std::pair<LabelIdx, LabelIdx> labels) {
    if (!IsInFuncWrapLabels(func)) {
      funcWrapLabels[func] = labels;
    }
  }

  static std::map<MIRFunction*, std::pair<LabelIdx, LabelIdx>> &GetFuncWrapLabels() {
    return funcWrapLabels;
  }
  static void SetCurCGFunc(CGFunc &cgFunc) {
    currentCGFunction = &cgFunc;
  }

  static const CGFunc *GetCurCGFunc() {
    return currentCGFunction;
  }

  static CGFunc *GetCurCGFuncNoConst() {
    return currentCGFunction;
  }

  virtual const InsnDesc &GetTargetMd(MOperator mOp) const = 0;
  virtual bool IsEffectiveCopy(Insn &insn) const = 0;
  virtual bool IsTargetInsn(MOperator mOp) const = 0;
  virtual bool IsClinitInsn(MOperator mOp) const = 0;
  virtual void DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const = 0;

 protected:
  const MIRModule *GetMIRModule() const {
    return mirModule;
  }

  MemPool *memPool = nullptr;
  MapleAllocator allocator;

 private:
  MIRModule *mirModule = nullptr;
  Emitter *emitter = nullptr;
  LabelIDOrder labelOrderCnt;
  static CGFunc *currentCGFunction;  /* current cg function being compiled */
  CGOptions cgOption;
  MIRSymbol *dbgTraceEnter = nullptr;
  MIRSymbol *dbgTraceExit = nullptr;
  MIRSymbol *dbgFuncProfile = nullptr;
  MIRSymbol *fileGP;  /* for lmbc, one local %GP per file */
  static std::map<MIRFunction *, std::pair<LabelIdx, LabelIdx>> funcWrapLabels;
  bool isLibcore = false;
  bool isLmbc = false;
};  /* class CG */
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CG_H */
