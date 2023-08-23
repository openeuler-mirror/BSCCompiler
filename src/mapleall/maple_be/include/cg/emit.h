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
#ifndef MAPLEBE_INCLUDE_CG_EMIT_H
#define MAPLEBE_INCLUDE_CG_EMIT_H

/* C++ headers */
#include <fstream>
#include <functional>
#include <map>
#include <array>
#include "isa.h"
#include "lsda.h"
#include "asm_info.h"
#include "cg.h"

/* Maple IR headers */
#include "mir_module.h"
#include "mir_const.h"
#include "mempool_allocator.h"
#include "muid_replacement.h"
#include "namemangler.h"
#include "debug_info.h"
#include "alignment.h"

#if defined(TARGRISCV64) && TARGRISCV64
#define CMNT "\t# "
#else
#define CMNT "\t// "
#endif
#define TEXT_BEGIN text0
#define TEXT_END etext0
#define DEBUG_INFO_0 debug_info0
#define DEBUG_ABBREV_0 debug_abbrev0
#define DEBUG_LINE_0 debug_line0
#define DEBUG_STR_LABEL ASF

namespace maplebe {
constexpr int32 kSizeOfDecoupleStaticStruct = 4;
constexpr uint32 kHugeSoInsnCountThreshold = 0x1f00000; /* 124M (4bytes per Insn), leave 4M rooms for 128M */
constexpr char kHugeSoPostFix[] = "$$hugeso_";
constexpr char kDebugMapleThis[] = "_this";
constexpr uint32 kDwarfVersion = 4;
constexpr uint32 kSizeOfPTR = 8;
class StructEmitInfo {
 public:
  /* default ctor */
  StructEmitInfo() = default;

  ~StructEmitInfo() = default;

  uint16 GetNextFieldOffset() const {
    return nextFieldOffset;
  }

  void SetNextFieldOffset(uint16 offset) {
    nextFieldOffset = offset;
  }

  void IncreaseNextFieldOffset(uint16 value) {
    nextFieldOffset += value;
  }

  uint8 GetCombineBitFieldWidth() const {
    return combineBitFieldWidth;
  }

  void SetCombineBitFieldWidth(uint8 offset) {
    combineBitFieldWidth = offset;
  }

  void IncreaseCombineBitFieldWidth(uint8 value) {
    combineBitFieldWidth += value;
  }

  void DecreaseCombineBitFieldWidth(uint8 value) {
    combineBitFieldWidth -= value;
  }

  uint64 GetCombineBitFieldValue() const {
    return combineBitFieldValue;
  }

  void SetCombineBitFieldValue(uint64 value) {
    combineBitFieldValue = value;
  }

  uint64 GetTotalSize() const {
    return totalSize;
  }

  void SetTotalSize(uint64 value) {
    totalSize = value;
  }

  void IncreaseTotalSize(uint64 value) {
    totalSize += value;
  }

 private:
  /* Next field offset in struct. */
  uint16 nextFieldOffset = 0;
  uint8 combineBitFieldWidth = 0;
  uint64 combineBitFieldValue = 0;
  /* Total size emitted in current struct. */
  uint64 totalSize = 0;
};

class FuncEmitInfo {
 public:
  CGFunc &GetCGFunc() {
    return cgFunc;
  }

  const CGFunc &GetCGFunc() const {
    return cgFunc;
  }

 protected:
  explicit FuncEmitInfo(CGFunc &func) : cgFunc(func) {}
  ~FuncEmitInfo() = default;

 private:
  CGFunc &cgFunc;
};

class Emitter {
 public:
  void CloseOutput() {
    if (outStream.is_open()) {
      outStream.close();
    }
    rangeIdx2PrefixStr.clear();
    hugeSoTargets.clear();
    labdie2labidxTable.clear();
    fileMap.clear();
  }

  MOperator GetCurrentMOP() const {
    return currentMop;
  }

  void SetCurrentMOP(const MOperator &mOp) {
    currentMop = mOp;
  }

  void EmitAsmLabel(AsmLabel label);
  void EmitAsmLabel(const MIRSymbol &mirSymbol, AsmLabel label);
  void EmitFileInfo(const std::string &fileName);
  /* a symbol start/end a block */
  void EmitBlockMarker(const std::string &markerName, const std::string &sectionName,
                       bool withAddr, const std::string &addrName = "");
  void EmitNullConstant(uint64 size);
  void EmitCombineBfldValue(StructEmitInfo &structEmitInfo, bool finished);
  void EmitBitFieldConstant(StructEmitInfo &structEmitInfo, MIRConst &mirConst, const MIRType *nextType,
                            uint64 fieldOffset);
  void EmitScalarConstant(MIRConst &mirConst, bool newLine = true, bool flag32 = false, bool isIndirect = false);
  void EmitStr(const std::string& mplStr, bool emitAscii = false, bool emitNewline = false);
  void EmitStrConstant(const MIRStrConst &mirStrConst, bool isIndirect = false);
  void EmitStr16Constant(const MIRStr16Const &mirStr16Const);
  void EmitIntConst(const MIRSymbol &mirSymbol, MIRAggConst &aggConst, uint32 itabConflictIndex,
                    const std::map<GStrIdx, MIRType*> &strIdx2Type, size_t idx);
  void EmitAddrofFuncConst(const MIRSymbol &mirSymbol, MIRConst &elemConst, size_t idx);
  void EmitAddrofSymbolConst(const MIRSymbol &mirSymbol, MIRConst &elemConst, size_t idx);
  void EmitConstantTable(const MIRSymbol &mirSymbol, MIRConst &mirConst,
                         const std::map<GStrIdx, MIRType*> &strIdx2Type);
  void EmitClassInfoSequential(const MIRSymbol &mirSymbol, const std::map<GStrIdx, MIRType*> &strIdx2Type,
                               const std::string &sectionName);
  void EmitMethodFieldSequential(const MIRSymbol &mirSymbol, const std::map<GStrIdx, MIRType*> &strIdx2Type,
                                 const std::string &sectionName);
  void EmitLiterals(std::vector<std::pair<MIRSymbol*, bool>> &literals,
                    const std::map<GStrIdx, MIRType*> &strIdx2Type);
  void EmitFuncLayoutInfo(const MIRSymbol &layout);
  void EmitGlobalVars(std::vector<std::pair<MIRSymbol*, bool>> &globalVars);
  void EmitGlobalVar(const MIRSymbol &globalVar);
  void EmitStaticFields(const std::vector<MIRSymbol*> &fields);
  void EmitLiteral(const MIRSymbol &literal, const std::map<GStrIdx, MIRType*> &strIdx2Type);
  void EmitStringPointers();
  void EmitStringSectionAndAlign(bool isTermByZero);
  void GetHotAndColdMetaSymbolInfo(const std::vector<MIRSymbol*> &mirSymbolVec,
                                   std::vector<MIRSymbol*> &hotFieldInfoSymbolVec,
                                   std::vector<MIRSymbol*> &coldFieldInfoSymbolVec, const std::string &prefixStr,
                                   bool forceCold = false) const;
  void EmitMetaDataSymbolWithMarkFlag(const std::vector<MIRSymbol*> &mirSymbolVec,
                                      const std::map<GStrIdx, MIRType*> &strIdx2Type,
                                      const std::string &prefixStr, const std::string &sectionName,
                                      bool isHotFlag);
  void EmitMethodDeclaringClass(const MIRSymbol &mirSymbol, const std::string &sectionName);
  void MarkVtabOrItabEndFlag(const std::vector<MIRSymbol*> &mirSymbolVec) const;
  void EmitArrayConstant(MIRConst &mirConst);
  void EmitStructConstant(MIRConst &mirConst);
  void EmitStructConstant(MIRConst &mirConst, uint32 &subStructFieldCounts);
  void EmitVectorConstant(MIRConst &mirConst);
  void EmitLocalVariable(const CGFunc &cgFunc);
  void EmitUninitializedSymbolsWithPrefixSection(const MIRSymbol &symbol, const std::string &sectionName);
  void EmitUninitializedSymbol(const MIRSymbol &mirSymbol);
  void EmitGlobalVariable();
  void EmitGlobalRootList(const MIRSymbol &mirSymbol);
  void EmitMuidTable(const std::vector<MIRSymbol*> &vec, const std::map<GStrIdx, MIRType*> &strIdx2Type,
                     const std::string &sectionName);
  MIRAddroffuncConst *GetAddroffuncConst(const MIRSymbol &mirSymbol, MIRAggConst &aggConst) const;
  int64 GetFieldOffsetValue(const std::string &className, const MIRIntConst &intConst,
                            const std::map<GStrIdx, MIRType*> &strIdx2Type) const;

  Emitter &Emit(int64 val) {
    outStream << val;
    return *this;
  }

  Emitter &Emit(const IntVal& val) {
    outStream << val.GetExtValue();
    return *this;
  }

  Emitter &Emit(const MapleString &str) {
    ASSERT(str.c_str() != nullptr, "nullptr check");
    outStream << str;
    return *this;
  }

  Emitter &Emit(const std::string &str) {
    outStream << str;
    return *this;
  }

  // provide anchor in specific postion for better assembly
  void InsertAnchor(const std::string &anchorName, int64 offset);
  void EmitLabelRef(LabelIdx labIdx);
  void EmitStmtLabel(LabelIdx labIdx);
  void EmitLabelPair(const LabelPair &pairLabel);
  void EmitLabelForFunc(const MIRFunction *func, LabelIdx labIdx);

  /* Emit signed/unsigned integer literals in decimal or hexadecimal */
  void EmitDecSigned(int64 num);
  void EmitDecUnsigned(uint64 num);
  void EmitHexUnsigned(uint64 num);

  /* Dwarf debug info */
  void FillInClassByteSize(DBGDie *die, DBGDieAttr *byteSizeAttr) const;
  void SetupDBGInfo(DebugInfo *mirdi);
  void ApplyInPrefixOrder(DBGDie *die, const std::function<void(DBGDie*)> &func);
  void AddLabelDieToLabelIdxMapping(DBGDie *lblDie, LabelIdx lblIdx);
  LabelIdx GetLabelIdxForLabelDie(DBGDie *lblDie);
  void EmitDIHeader();
  void EmitDIFooter();
  void EmitDIHeaderFileInfo();
  void EmitDIDebugInfoSection(DebugInfo *mirdi);
  void EmitDIDebugAbbrevSection(DebugInfo *mirdi);
  void EmitDIDebugARangesSection();
  void EmitDIDebugRangesSection();
  void EmitDIDebugLineSection();
  void EmitDIDebugStrSection();
  MIRFunction *GetDwTagSubprogram(const MapleVector<DBGDieAttr*> &attrvec, DebugInfo &di) const;
  void EmitDIAttrValue(DBGDie *die, DBGDieAttr *attr, DwAt attrName, DwTag tagName, DebugInfo *di);
  void EmitDIFormSpecification(unsigned int dwform);
  void EmitDIFormSpecification(const DBGDieAttr *attr) {
    EmitDIFormSpecification(attr->GetDwForm());
  }

#if 1 /* REQUIRE TO SEPERATE TARGAARCH64 TARGARM32 */
/* Following code is under TARGAARCH64 condition */
  void EmitHugeSoRoutines(bool lastRoutine = false);
  void EmitInlineAsmSection();

  uint64 GetJavaInsnCount() const {
    return javaInsnCount;
  }

  uint64 GetFuncInsnCount() const {
    return funcInsnCount;
  }

  MapleMap<uint32_t, std::string> &GetFileMap() {
    return fileMap;
  }

  void SetFileMapValue(uint32_t n, const std::string &file) {
    fileMap[n] = file;
  }

  CG *GetCG() const {
    return cg;
  }

  void ClearFuncInsnCount() {
    funcInsnCount = 0;
  }

  void IncreaseJavaInsnCount(uint64 n = 1, bool alignToQuad = false) {
    if (alignToQuad) {
      javaInsnCount = (javaInsnCount + 1) & (~0x1UL);
      funcInsnCount = (funcInsnCount + 1) & (~0x1UL);
    }
    javaInsnCount += n;
    funcInsnCount += n;
#ifdef EMIT_INSN_COUNT
    Emit(" /* InsnCount: ");
    Emit(javaInsnCount *);
    Emit("*/ ");
#endif
  }

  bool NeedToDealWithHugeSo() const {
    return javaInsnCount > kHugeSoInsnCountThreshold;
  }

  std::string HugeSoPostFix() const {
    return std::string(kHugeSoPostFix) + std::to_string(hugeSoSeqence);
  }

  void InsertHugeSoTarget(const std::string &target) {
    (void)hugeSoTargets.insert(target);
  }
#endif

  void InsertLabdie2labidxTable(DBGDie *lbldie, LabelIdx lab) {
    if (labdie2labidxTable.find(lbldie) == labdie2labidxTable.end()) {
      labdie2labidxTable[lbldie] = lab;
    }
  }

 protected:
  Emitter(CG &cg, const std::string &fileName)
      : cg(&cg),
        rangeIdx2PrefixStr(cg.GetMIRModule()->GetMPAllocator().Adapter()),
        arraySize(0),
        isFlexibleArray(false),
        stringPtr(cg.GetMIRModule()->GetMPAllocator().Adapter()),
        localStrPtr(cg.GetMIRModule()->GetMPAllocator().Adapter()),
        hugeSoTargets(cg.GetMIRModule()->GetMPAllocator().Adapter()),
        labdie2labidxTable(std::less<DBGDie*>(), cg.GetMIRModule()->GetMPAllocator().Adapter()),
        fileMap(std::less<uint32_t>(), cg.GetMIRModule()->GetMPAllocator().Adapter()),
        globalTlsDataVec(cg.GetMIRModule()->GetMPAllocator().Adapter()),
        globalTlsBssVec(cg.GetMIRModule()->GetMPAllocator().Adapter()) {
    outStream.open(fileName, std::ios::trunc);
    MIRModule &mirModule = *cg.GetMIRModule();
    memPool = mirModule.GetMemPool();
    asmInfo = memPool->New<AsmInfo>(*memPool);
  }

  ~Emitter() = default;

 private:
  AsmLabel GetTypeAsmInfoName(PrimType primType) const;
  void EmitDWRef(const std::string &name);
  void InitRangeIdx2PerfixStr();
  void EmitAddressString(const std::string &address);
  void EmitAliasAndRef(const MIRSymbol &sym); // handle function symbol which has alias and weak ref
  // collect all global TLS together -- better perfomance for local dynamic
  void EmitTLSBlock(const MapleVector<MIRSymbol*> &tdataVec, const MapleVector<MIRSymbol*> &tbssVec);

  CG *cg;
  MOperator currentMop = UINT_MAX;
  MapleUnorderedMap<int, std::string> rangeIdx2PrefixStr;
  const AsmInfo *asmInfo = nullptr;
  std::ofstream outStream;
  MemPool *memPool = nullptr;
  size_t arraySize;
  bool isFlexibleArray;
  MapleSet<UStrIdx> stringPtr;
  MapleVector<UStrIdx> localStrPtr;
#if 1 /* REQUIRE TO SEPERATE TARGAARCH64 TARGARM32 */
/* Following code is under TARGAARCH64 condition */
  uint64 javaInsnCount = 0;
  uint64 funcInsnCount = 0;
  MapleSet<std::string> hugeSoTargets;
  uint32 hugeSoSeqence = 2;
#endif
  MapleMap<DBGDie*, LabelIdx> labdie2labidxTable;
  MapleMap<uint32_t, std::string> fileMap;

  // for global warmup localDynamicOpt
  MapleVector<MIRSymbol*> globalTlsDataVec;
  MapleVector<MIRSymbol*> globalTlsBssVec;
};

class OpndEmitVisitor : public OperandVisitorBase,
                        public OperandVisitors<RegOperand,
                                               ImmOperand,
                                               MemOperand,
                                               OfstOperand,
                                               ListOperand,
                                               LabelOperand,
                                               FuncNameOperand,
                                               StImmOperand,
                                               CondOperand,
                                               BitShiftOperand,
                                               ExtendShiftOperand,
                                               CommentOperand> {
 public:
  explicit OpndEmitVisitor(Emitter &asmEmitter, const OpndDesc *operandProp)
      : emitter(asmEmitter),
        opndProp(operandProp) {}
  ~OpndEmitVisitor() override {
    opndProp = nullptr;
  }
  uint8 GetSlot() const {
    return slot;
  }
  void SetSlot(uint8 startIndex) {
    slot = startIndex;
  }
 protected:
  Emitter &emitter;
  /* start index of InsnDesc.opndMD[],used for memOperand and listOperand.
   * if Opnd is memOperand:
   * (arm64)
   * str x0, [x1, x2]        ----> slot = 1
   * stp x1, x0, [sp, #8]    ----> slot = 2
   * (x64)
   * leaq 8(%rip), %rax      ----> slot = 0
   *
   * if opnd is listOperand:
   * asm {destReg, {srcReg0, srcReg1, srcReg2 ...}}  -----> slot = 1
   * asm {{destReg0, destReg2 ...}, srcReg}          -----> slot = 0
   */
  uint8 slot = 255;
  const OpndDesc *opndProp;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_EMIT_H */
