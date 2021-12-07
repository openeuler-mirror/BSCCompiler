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
#ifndef HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_INTERFACE_H
#define HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_INTERFACE_H

#include <cstdint>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>
#include <memory>
#include <list>
#include <functional>
#include "namemangler.h"

namespace  maple {
enum ValueType {
  kByte = 0x00,
  kShort = 0x02,
  kChar = 0x03,
  kInt = 0x04,
  kLong = 0x06,
  kFloat = 0x10,
  kDouble = 0x11,
  kMethodType = 0x15,
  kMethodHandle = 0x16,
  kString = 0x17,
  kType = 0x18,
  kField = 0x19,
  kMethod = 0x1a,
  kEnum = 0x1b,
  kArray = 0x1c,
  kAnnotation = 0x1d,
  kNull = 0x1e,
  kBoolean = 0x1f
};
const uint16_t kIDexNumPackedOpcodes = 0x100;
const uint16_t kIDexPackedSwitchSignature = 0x100;
const uint16_t kIDexSparseSwitchSignature = 0x200;

// opcode in dex file
enum IDexOpcode : uint8_t {
  kOpNop                          = 0x00,
  kOpMove                         = 0x01,
  kOpMoveFrom16                   = 0x02,
  kOpMove16                       = 0x03,
  kOpMoveWide                     = 0x04,
  kOpMoveWideFrom16               = 0x05,
  kOpMoveWide16                   = 0x06,
  kOpMoveObject                   = 0x07,
  kOpMoveObjectFrom16             = 0x08,
  kOpMoveObject16                 = 0x09,
  kOpMoveResult                   = 0x0a,
  kOpMoveResultWide               = 0x0b,
  kOpMoveResultObject             = 0x0c,
  kOpMoveException                = 0x0d,
  kOpReturnVoid                   = 0x0e,
  kOpReturn                       = 0x0f,
  kOpReturnWide                   = 0x10,
  kOpReturnObject                 = 0x11,
  kOpConst4                       = 0x12,
  kOpConst16                      = 0x13,
  kOpConst                        = 0x14,
  kOpConstHigh16                  = 0x15,
  kOpConstWide16                  = 0x16,
  kOpConstWide32                  = 0x17,
  kOpConstWide                    = 0x18,
  kOpConstWideHigh16              = 0x19,
  kOpConstString                  = 0x1a,
  kOpConstStringJumbo             = 0x1b,
  kOpConstClass                   = 0x1c,
  kOpMonitorEnter                 = 0x1d,
  kOpMonitorExit                  = 0x1e,
  kOpCheckCast                    = 0x1f,
  kOpInstanceOf                   = 0x20,
  kOpArrayLength                  = 0x21,
  kOpNewInstance                  = 0x22,
  kOpNewArray                     = 0x23,
  kOpFilledNewArray               = 0x24,
  kOpFilledNewArrayRange          = 0x25,
  kOpFillArrayData                = 0x26,
  kOpThrow                        = 0x27,
  kOpGoto                         = 0x28,
  kOpGoto16                       = 0x29,
  kOpGoto32                       = 0x2a,
  kOpPackedSwitch                 = 0x2b,
  kOpSparseSwitch                 = 0x2c,
  kOpCmplFloat                    = 0x2d,
  kOpCmpgFloat                    = 0x2e,
  kOpCmplDouble                   = 0x2f,
  kOpCmpgDouble                   = 0x30,
  kOpCmpLong                      = 0x31,
  kOpIfEq                         = 0x32,
  kOpIfNe                         = 0x33,
  kOpIfLt                         = 0x34,
  kOpIfGe                         = 0x35,
  kOpIfGt                         = 0x36,
  kOpIfLe                         = 0x37,
  kOpIfEqz                        = 0x38,
  kOpIfNez                        = 0x39,
  kOpIfLtz                        = 0x3a,
  kOpIfGez                        = 0x3b,
  kOpIfGtz                        = 0x3c,
  kOpIfLez                        = 0x3d,
  kOpUnused3E                     = 0x3e,
  kOpUnused3F                     = 0x3f,
  kOpUnused40                     = 0x40,
  kOpUnused41                     = 0x41,
  kOpUnused42                     = 0x42,
  kOpUnused43                     = 0x43,
  kOpAget                         = 0x44,
  kOpAgetWide                     = 0x45,
  kOpAgetObject                   = 0x46,
  kOpAgetBoolean                  = 0x47,
  kOpAgetByte                     = 0x48,
  kOpAgetChar                     = 0x49,
  kOpAgetShort                    = 0x4a,
  kOpAput                         = 0x4b,
  kOpAputWide                     = 0x4c,
  kOpAputObject                   = 0x4d,
  kOpAputBoolean                  = 0x4e,
  kOpAputByte                     = 0x4f,
  kOpAputChar                     = 0x50,
  kOpAputShort                    = 0x51,
  kOpIget                         = 0x52,
  kOpIgetWide                     = 0x53,
  kOpIgetObject                   = 0x54,
  kOpIgetBoolean                  = 0x55,
  kOpIgetByte                     = 0x56,
  kOpIgetChar                     = 0x57,
  kOpIgetShort                    = 0x58,
  kOpIput                         = 0x59,
  kOpIputWide                     = 0x5a,
  kOpIputObject                   = 0x5b,
  kOpIputBoolean                  = 0x5c,
  kOpIputByte                     = 0x5d,
  kOpIputChar                     = 0x5e,
  kOpIputShort                    = 0x5f,
  kOpSget                         = 0x60,
  kOpSgetWide                     = 0x61,
  kOpSgetObject                   = 0x62,
  kOpSgetBoolean                  = 0x63,
  kOpSgetByte                     = 0x64,
  kOpSgetChar                     = 0x65,
  kOpSgetShort                    = 0x66,
  kOpSput                         = 0x67,
  kOpSputWide                     = 0x68,
  kOpSputObject                   = 0x69,
  kOpSputBoolean                  = 0x6a,
  kOpSputByte                     = 0x6b,
  kOpSputChar                     = 0x6c,
  kOpSputShort                    = 0x6d,
  kOpInvokeVirtual                = 0x6e,
  kOpInvokeSuper                  = 0x6f,
  kOpInvokeDirect                 = 0x70,
  kOpInvokeStatic                 = 0x71,
  kOpInvokeInterface              = 0x72,
  kOpUnused73                     = 0x73,
  kOpInvokeVirtualRange           = 0x74,
  kOpInvokeSuperRange             = 0x75,
  kOpInvokeDirectRange            = 0x76,
  kOpInvokeStaticRange            = 0x77,
  kOpInvokeInterfaceRange         = 0x78,
  kOpUnused79                     = 0x79,
  kOpUnused7A                     = 0x7a,
  kOpNegInt                       = 0x7b,
  kOpNotInt                       = 0x7c,
  kOpNegLong                      = 0x7d,
  kOpNotLong                      = 0x7e,
  kOpNegFloat                     = 0x7f,
  kOpNegDouble                    = 0x80,
  kOpIntToLong                    = 0x81,
  kOpIntToFloat                   = 0x82,
  kOpIntToDouble                  = 0x83,
  kOpLongToInt                    = 0x84,
  kOpLongToFloat                  = 0x85,
  kOpLongToDouble                 = 0x86,
  kOpFloatToInt                   = 0x87,
  kOpFloatToLong                  = 0x88,
  kOpFloatToDouble                = 0x89,
  kOpDoubleToInt                  = 0x8a,
  kOpDoubleToLong                 = 0x8b,
  kOpDoubleToFloat                = 0x8c,
  kOpIntToByte                    = 0x8d,
  kOpIntToChar                    = 0x8e,
  kOpIntToShort                   = 0x8f,
  kOpAddInt                       = 0x90,
  kOpSubInt                       = 0x91,
  kOpMulInt                       = 0x92,
  kOpDivInt                       = 0x93,
  kOpRemInt                       = 0x94,
  kOpAndInt                       = 0x95,
  kOpOrInt                        = 0x96,
  kOpXorInt                       = 0x97,
  kOpShlInt                       = 0x98,
  kOpShrInt                       = 0x99,
  kOpUshrInt                      = 0x9a,
  kOpAddLong                      = 0x9b,
  kOpSubLong                      = 0x9c,
  kOpMulLong                      = 0x9d,
  kOpDivLong                      = 0x9e,
  kOpRemLong                      = 0x9f,
  kOpAndLong                      = 0xa0,
  kOpOrLong                       = 0xa1,
  kOpXorLong                      = 0xa2,
  kOpShlLong                      = 0xa3,
  kOpShrLong                      = 0xa4,
  kOpUshrLong                     = 0xa5,
  kOpAddFloat                     = 0xa6,
  kOpSubFloat                     = 0xa7,
  kOpMulFloat                     = 0xa8,
  kOpDivFloat                     = 0xa9,
  kOpRemFloat                     = 0xaa,
  kOpAddDouble                    = 0xab,
  kOpSubDouble                    = 0xac,
  kOpMulDouble                    = 0xad,
  kOpDivDouble                    = 0xae,
  kOpRemDouble                    = 0xaf,
  kOpAddInt2Addr                  = 0xb0,
  kOpSubInt2Addr                  = 0xb1,
  kOpMulInt2Addr                  = 0xb2,
  kOpDivInt2Addr                  = 0xb3,
  kOpRemInt2Addr                  = 0xb4,
  kOpAndInt2Addr                  = 0xb5,
  kOpOrInt2Addr                   = 0xb6,
  kOpXorInt2Addr                  = 0xb7,
  kOpShlInt2Addr                  = 0xb8,
  kOpShrInt2Addr                  = 0xb9,
  kOpUshrInt2Addr                 = 0xba,
  kOpAddLong2Addr                 = 0xbb,
  kOpSubLong2Addr                 = 0xbc,
  kOpMulLong2Addr                 = 0xbd,
  kOpDivLong2Addr                 = 0xbe,
  kOpRemLong2Addr                 = 0xbf,
  kOpAndLong2Addr                 = 0xc0,
  kOpOrLong2Addr                  = 0xc1,
  kOpXorLong2Addr                 = 0xc2,
  kOpShlLong2Addr                 = 0xc3,
  kOpShrLong2Addr                 = 0xc4,
  kOpUshrLong2Addr                = 0xc5,
  kOpAddFloat2Addr                = 0xc6,
  kOpSubFloat2Addr                = 0xc7,
  kOpMulFloat2Addr                = 0xc8,
  kOpDivFloat2Addr                = 0xc9,
  kOpRemFloat2Addr                = 0xca,
  kOpAddDouble2Addr               = 0xcb,
  kOpSubDouble2Addr               = 0xcc,
  kOpMulDouble2Addr               = 0xcd,
  kOpDivDouble2Addr               = 0xce,
  kOpRemDouble2Addr               = 0xcf,
  kOpAddIntLit16                  = 0xd0,
  kOpRsubInt                      = 0xd1,
  kOpMulIntLit16                  = 0xd2,
  kOpDivIntLit16                  = 0xd3,
  kOpRemIntLit16                  = 0xd4,
  kOpAndIntLit16                  = 0xd5,
  kOpOrIntLit16                   = 0xd6,
  kOpXorIntLit16                  = 0xd7,
  kOpAddIntLit8                   = 0xd8,
  kOpRsubIntLit8                  = 0xd9,
  kOpMulIntLit8                   = 0xda,
  kOpDivIntLit8                   = 0xdb,
  kOpRemIntLit8                   = 0xdc,
  kOpAndIntLit8                   = 0xdd,
  kOpOrIntLit8                    = 0xde,
  kOpXorIntLit8                   = 0xdf,
  kOpShlIntLit8                   = 0xe0,
  kOpShrIntLit8                   = 0xe1,
  kOpUshrIntLit8                  = 0xe2,
  kOpIgetVolatile                 = 0xe3,
  kOpIputVolatile                 = 0xe4,
  kOpSgetVolatile                 = 0xe5,
  kOpSputVolatile                 = 0xe6,
  kOpIgetObjectVolatile           = 0xe7,
  kOpIgetWideVolatile             = 0xe8,
  kOpIputWideVolatile             = 0xe9,
  kOpSgetWideVolatile             = 0xea,
  kOpSputWideVolatile             = 0xeb,
  kOpBreakpoint                   = 0xec,
  kOpThrowVerificationError       = 0xed,
  kOpExecuteInline                = 0xee,
  kOpExecuteInlineRange           = 0xef,
  kOpInvokeObjectInitRange        = 0xf0,
  kOpReturnVoidBarrier            = 0xf1,
  kOpIgetQuick                    = 0xf2,
  kOpIgetWideQuick                = 0xf3,
  kOpIgetObjectQuick              = 0xf4,
  kOpIputQuick                    = 0xf5,
  kOpIputWideQuick                = 0xf6,
  kOpIputObjectQuick              = 0xf7,
  kOpInvokeVirtualQuick           = 0xf8,
  kOpInvokeVirtualQuickRange      = 0xf9,
  kOpInvokePolymorphic            = 0xfa,
  kOpInvokePolymorphicRange       = 0xfb,
  kOpInvokeCustom                 = 0xfc,
  kOpInvokeCustomRange            = 0xfd,
  kOpSputObjectVolatile           = 0xfe,
  kOpUnusedFF                     = 0xff
};

enum IDexInstructionIndexType : uint8_t {
  kIDexIndexUnknown = 0,
  kIDexIndexNone,               // has no index
  kIDexIndexTypeRef,            // type reference index
  kIDexIndexStringRef,          // string reference index
  kIDexIndexMethodRef,          // method reference index
  kIDexIndexFieldRef,           // field reference index
  kIDexIndexFieldOffset,        // field offset (for static linked fields)
  kIDexIndexVtableOffset,       // vtable offset (for static linked methods)
  kIDexIndexMethodAndProtoRef,  // method index and proto indexv
  kIDexIndexCallSiteRef,        // call site index
  kIDexIndexInlineMethod,       // inline method index (for inline linked methods)
  kIDexIndexVaries              // "It depends." Used for throw-verification-error
};

enum IDexInstructionFormat : uint8_t {
  kIDexFmt10x,      // op
  kIDexFmt12x,      // op vA, vB
  kIDexFmt11n,      // op vA, #+B
  kIDexFmt11x,      // op vAA
  kIDexFmt10t,      // op +AA
  kIDexFmt20t,      // op +AAAA
  kIDexFmt22x,      // op vAA, vBBBB
  kIDexFmt21t,      // op vAA, +BBBB
  kIDexFmt21s,      // op vAA, #+BBBB
  kIDexFmt21h,      // op vAA, #+BBBB00000[00000000]
  kIDexFmt21c,      // op vAA, thing@BBBB
  kIDexFmt23x,      // op vAA, vBB, vCC
  kIDexFmt22b,      // op vAA, vBB, #+CC
  kIDexFmt22t,      // op vA, vB, +CCCC
  kIDexFmt22s,      // op vA, vB, #+CCCC
  kIDexFmt22c,      // op vA, vB, thing@CCCC
  kIDexFmt32x,      // op vAAAA, vBBBB
  kIDexFmt30t,      // op +AAAAAAAA
  kIDexFmt31t,      // op vAA, +BBBBBBBB
  kIDexFmt31i,      // op vAA, #+BBBBBBBB
  kIDexFmt31c,      // op vAA, string@BBBBBBBB
  kIDexFmt35c,      // op {vC,vD,vE,vF,vG}, thing@BBBB
  kIDexFmt3rc,      // op {vCCCC .. v(CCCC+AA-1)}, thing@BBBB
  kIDexFmt45cc,     // op {vC, vD, vE, vF, vG}, meth@BBBB, proto@HHHH
  kIDexFmt4rcc,     // op {VCCCC .. v(CCCC+AA-1)}, meth@BBBB, proto@HHHH
  kIDexFmt51l       // op vAA, #+BBBBBBBBBBBBBBBB
};

class IDexFile;

class IDexInstruction {
 public:
  IDexInstruction() = default;
  virtual ~IDexInstruction() = default;
  virtual IDexOpcode GetOpcode() const = 0;
  virtual const char *GetOpcodeName() const = 0;
  virtual uint32_t GetVRegA() const = 0;
  virtual uint32_t GetVRegB() const = 0;
  virtual uint64_t GetVRegBWide() const = 0;
  virtual uint32_t GetVRegC() const = 0;
  virtual uint32_t GetVRegH() const = 0;
  virtual uint32_t GetArg(uint32_t index) const = 0;
  virtual bool HasVRegA() const = 0;
  virtual bool HasVRegB() const = 0;
  virtual bool HasWideVRegB() const = 0;
  virtual bool HasVRegC() const = 0;
  virtual bool HasVRegH() const = 0;
  virtual bool HasArgs() const = 0;
  virtual IDexInstructionIndexType GetIndexType() const = 0;
  virtual IDexInstructionFormat GetFormat() const = 0;
  virtual size_t GetWidth() const = 0;
  virtual void SetOpcode(IDexOpcode opcode) = 0;
  virtual void SetVRegB(uint32_t vRegB) = 0;
  virtual void SetVRegBWide(uint64_t vRegBWide) = 0;
  virtual void SetIndexType(IDexInstructionIndexType indexType) = 0;
};

class IDexCatchHandlerItem {
 public:
  IDexCatchHandlerItem(uint32_t typeIdx, uint32_t address) : typeIdx(typeIdx), address(address) {}
  ~IDexCatchHandlerItem() = default;
  IDexCatchHandlerItem(const IDexCatchHandlerItem &item) = default;
  IDexCatchHandlerItem &operator=(const IDexCatchHandlerItem&) = default;
  uint32_t GetHandlerTypeIdx() const;
  uint32_t GetHandlerAddress() const;
  bool IsCatchAllHandlerType() const;

 private:
  uint32_t typeIdx;  // input parameter
  uint32_t address;  // input parameter
};

class IDexTryItem {
 public:
  static const IDexTryItem *GetInstance(const void *item);
  uint32_t GetStartAddr() const;
  uint32_t GetEndAddr() const;
  void GetCatchHandlerItems(const IDexFile &dexFile, uint32_t codeOff,
                            std::vector<IDexCatchHandlerItem> &items) const;

 private:
  IDexTryItem() = default;
  virtual ~IDexTryItem() = default;
};

class IDexProtoIdItem {
 public:
  static const IDexProtoIdItem *GetInstance(const IDexFile &dexFile, uint32_t index);
  const char *GetReturnTypeName(const IDexFile &dexFile) const;
  uint32_t GetParameterTypeSize(const IDexFile &dexFile) const;
  void GetParameterTypeIndexes(const IDexFile &dexFile, std::vector<uint16_t> &indexes) const;
  std::string GetDescriptor(const IDexFile &dexFile) const;
  const char *GetShorty(const IDexFile &dexFile) const;

 private:
  IDexProtoIdItem() = default;
  virtual ~IDexProtoIdItem() = default;
};

class IDexMethodIdItem {
 public:
  static const IDexMethodIdItem *GetInstance(const IDexFile &dexFile, uint32_t index);
  uint32_t GetClassIdx() const;
  const char *GetDefiningClassName(const IDexFile &dexFile) const;
  uint16_t GetProtoIdx() const;
  const IDexProtoIdItem *GetProtoIdItem(const IDexFile &dexFile) const;
  uint32_t GetNameIdx() const;
  const char *GetShortMethodName(const IDexFile &dexFile) const;
  std::string GetDescriptor(const IDexFile &dexFile) const;

 private:
  IDexMethodIdItem() = default;
  virtual ~IDexMethodIdItem() = default;
};

class IDexMethodItem {
 public:
  IDexMethodItem(uint32_t methodIdx, uint32_t accessFlags, uint32_t codeOff);
  uint32_t GetMethodCodeOff(const IDexMethodItem *item) const;
  virtual ~IDexMethodItem() = default;
  uint32_t GetMethodIdx() const;
  const IDexMethodIdItem *GetMethodIdItem(const IDexFile &dexFile) const;
  uint32_t GetAccessFlags() const;
  bool HasCode(const IDexFile &dexFile) const;
  uint16_t GetRegistersSize(const IDexFile &dexFile) const;
  uint16_t GetInsSize(const IDexFile &dexFile) const;
  uint16_t GetOutsSize(const IDexFile &dexFile) const;
  uint16_t GetTriesSize(const IDexFile &dexFile) const;
  uint32_t GetInsnsSize(const IDexFile &dexFile) const;
  const uint16_t *GetInsnsByOffset(const IDexFile &dexFile, uint32_t offset) const;
  uint32_t GetCodeOff() const;
  bool IsStatic() const;
  void GetPCInstructionMap(const IDexFile &dexFile,
                           std::map<uint32_t, std::unique_ptr<IDexInstruction>> &pcInstructionMap) const;
  void GetTryItems(const IDexFile &dexFile, std::vector<const IDexTryItem*> &tryItems) const;
  void GetSrcPositionInfo(const IDexFile &dexFile, std::map<uint32_t, uint32_t> &srcPosInfo) const;
  void GetSrcLocalInfo(const IDexFile &dexFile,
                       std::map<uint16_t, std::set<std::tuple<std::string, std::string, std::string>>> &srcLocal) const;

 private:
  uint32_t methodIdx;           // input parameter
  uint32_t accessFlags;         // input parameter
  uint32_t codeOff;             // input parameter
};

class IDexFieldIdItem {
 public:
  static const IDexFieldIdItem *GetInstance(const IDexFile &dexFile, uint32_t index);
  uint32_t GetClassIdx() const;
  const char *GetDefiningClassName(const IDexFile &dexFile) const;
  uint32_t GetTypeIdx() const;
  const char *GetFieldTypeName(const IDexFile &dexFile) const;
  uint32_t GetNameIdx() const;
  const char *GetShortFieldName(const IDexFile &dexFile) const;

 private:
  IDexFieldIdItem() = default;
  virtual ~IDexFieldIdItem() = default;
};

class IDexFieldItem {
 public:
  IDexFieldItem(uint32_t fieldIndex, uint32_t accessFlags);
  virtual ~IDexFieldItem() = default;
  uint32_t GetFieldIdx() const;
  const IDexFieldIdItem *GetFieldIdItem(const IDexFile &dexFile) const;
  uint32_t GetAccessFlags() const;

 private:
  uint32_t fieldIndex;          // input parameter
  uint32_t accessFlags;         // input parameter
};

class IDexAnnotation {
 public:
  static const IDexAnnotation *GetInstance(const void *data);
  uint8_t GetVisibility() const;
  const uint8_t *GetAnnotationData() const;

 private:
  IDexAnnotation() = default;
  virtual ~IDexAnnotation() = default;
};

class IDexAnnotationSet {
 public:
  static const IDexAnnotationSet *GetInstance(const void *data);
  void GetAnnotations(const IDexFile &dexFile, std::vector<const IDexAnnotation*> &item) const;
  bool IsValid() const;

 private:
  IDexAnnotationSet() = default;
  virtual ~IDexAnnotationSet() = default;
};

class IDexAnnotationSetList {
 public:
  static const IDexAnnotationSetList *GetInstance(const void *data);
  void GetAnnotationSets(const IDexFile &dexFile, std::vector<const IDexAnnotationSet*> &items) const;

 private:
  IDexAnnotationSetList() = default;
  virtual ~IDexAnnotationSetList() = default;
};

class IDexFieldAnnotations {
 public:
  static const IDexFieldAnnotations *GetInstance(const void *data);
  const IDexFieldIdItem *GetFieldIdItem(const IDexFile &dexFile) const;
  const IDexAnnotationSet *GetAnnotationSet(const IDexFile &dexFile) const;
  uint32_t GetFieldIdx() const;

 private:
  IDexFieldAnnotations() = default;
  virtual ~IDexFieldAnnotations() = default;
};

class IDexMethodAnnotations {
 public:
  static const IDexMethodAnnotations *GetInstance(const void *data);
  const IDexMethodIdItem *GetMethodIdItem(const IDexFile &dexFile) const;
  const IDexAnnotationSet *GetAnnotationSet(const IDexFile &dexFile) const;
  uint32_t GetMethodIdx() const;

 private:
  IDexMethodAnnotations() = default;
  virtual ~IDexMethodAnnotations() = default;
};

class IDexParameterAnnotations {
 public:
  static const IDexParameterAnnotations *GetInstance(const void *data);
  const IDexMethodIdItem *GetMethodIdItem(const IDexFile &dexFile) const;
  const IDexAnnotationSetList *GetAnnotationSetList(const IDexFile &dexFile) const;
  uint32_t GetMethodIdx() const;

 private:
  IDexParameterAnnotations() = default;
  virtual ~IDexParameterAnnotations() = default;
};

class IDexAnnotationsDirectory {
 public:
  static const IDexAnnotationsDirectory *GetInstance(const void *data);
  bool HasClassAnnotationSet(const IDexFile &dexFile) const;
  bool HasFieldAnnotationsItems(const IDexFile &dexFile) const;
  bool HasMethodAnnotationsItems(const IDexFile &dexFile) const;
  bool HasParameterAnnotationsItems(const IDexFile &dexFile) const;
  const IDexAnnotationSet *GetClassAnnotationSet(const IDexFile &dexFile) const;
  void GetFieldAnnotationsItems(const IDexFile &dexFile,
                                std::vector<const IDexFieldAnnotations*> &items) const;
  void GetMethodAnnotationsItems(const IDexFile &dexFile,
                                 std::vector<const IDexMethodAnnotations*> &items) const;
  void GetParameterAnnotationsItems(const IDexFile &dexFile,
                                    std::vector<const IDexParameterAnnotations*> &items) const;

 private:
  IDexAnnotationsDirectory() = default;
  virtual ~IDexAnnotationsDirectory() = default;
};

class IDexClassItem {
 public:
  uint32_t GetClassIdx() const;
  const char *GetClassName(const IDexFile &dexFile) const;
  uint32_t GetAccessFlags() const;
  uint32_t GetSuperclassIdx() const;
  const char *GetSuperClassName(const IDexFile &dexFile) const;
  uint32_t GetInterfacesOff() const;
  void GetInterfaceTypeIndexes(const IDexFile &dexFile, std::vector<uint16_t> &indexes) const;
  void GetInterfaceNames(const IDexFile &dexFile, std::vector<const char*> &names) const;
  uint32_t GetSourceFileIdx() const;
  const char *GetJavaSourceFileName(const IDexFile &dexFile) const;
  bool HasAnnotationsDirectory(const IDexFile &dexFile) const;
  uint32_t GetAnnotationsOff() const;
  const IDexAnnotationsDirectory *GetAnnotationsDirectory(const IDexFile &dexFile) const;
  uint32_t GetClassDataOff() const;
  std::vector<std::unique_ptr<IDexFieldItem>> GetFields(const IDexFile &dexFile) const;
  bool HasStaticValuesList() const;
  const uint8_t *GetStaticValuesList(const IDexFile &dexFile) const;
  bool IsInterface() const;
  bool IsSuperclassValid() const;
  std::vector<std::pair<uint32_t, uint32_t>> GetMethodsIdxAndFlag(const IDexFile &dexFile, bool isVirtual) const;
  std::unique_ptr<IDexMethodItem> GetDirectMethod(const IDexFile &dexFile, uint32_t index) const;
  std::unique_ptr<IDexMethodItem> GetVirtualMethod(const IDexFile &dexFile, uint32_t index) const;

 private:
  IDexClassItem() = default;
  virtual ~IDexClassItem() = default;
};

class IDexHeader {
 public:
  static const IDexHeader *GetInstance(const IDexFile &dexFile);
  uint8_t GetMagic(uint32_t index) const;
  uint32_t GetChecksum() const;
  std::string GetSignature() const;
  uint32_t GetFileSize() const;
  uint32_t GetHeaderSize() const;
  uint32_t GetEndianTag() const;
  uint32_t GetLinkSize() const;
  uint32_t GetLinkOff() const;
  uint32_t GetMapOff() const;
  uint32_t GetStringIdsSize() const;
  uint32_t GetStringIdsOff() const;
  uint32_t GetTypeIdsSize() const;
  uint32_t GetTypeIdsOff() const;
  uint32_t GetProtoIdsSize() const;
  uint32_t GetProtoIdsOff() const;
  uint32_t GetFieldIdsSize() const;
  uint32_t GetFieldIdsOff() const;
  uint32_t GetMethodIdsSize() const;
  uint32_t GetMethodIdsOff() const;
  uint32_t GetClassDefsSize() const;
  uint32_t GetClassDefsOff() const;
  uint32_t GetDataSize() const;
  uint32_t GetDataOff() const;
  std::string GetDexVesion() const;

 private:
  IDexHeader() = default;
  virtual ~IDexHeader() = default;
};

class IDexMapList {
 public:
  static const IDexMapList *GetInstance(const void *data);
  uint32_t GetSize() const;
  uint16_t GetType(uint32_t index) const;
  uint32_t GetTypeSize(uint32_t index) const;

 private:
  IDexMapList() = default;
  virtual ~IDexMapList() = default;
};

class ResolvedMethodHandleItem;
class ResolvedMethodType {
 public:
  static const ResolvedMethodType *GetInstance(const IDexFile &dexFile, const ResolvedMethodHandleItem &item);
  static const ResolvedMethodType *GetInstance(const IDexFile &dexFile, uint32_t callSiteId);
  static void SignatureTypes(const std::string &mt, std::list<std::string> &types);
  static std::string SignatureReturnType(const std::string &mt);
  const std::string GetReturnType(const IDexFile &dexFile) const;
  const std::string GetRawType(const IDexFile &dexFile) const;
  void GetArgTypes(const IDexFile &dexFile, std::list<std::string> &types) const;
 private:
  ResolvedMethodType() = default;
  ~ResolvedMethodType() = default;
};

class ResolvedMethodHandleItem {
 public:
  static const ResolvedMethodHandleItem *GetInstance(const IDexFile &dexFile, uint32_t index);
  bool IsInstance() const;
  bool IsInvoke() const;
  const std::string GetDeclaringClass(const IDexFile &dexFile) const;
  const std::string GetMember(const IDexFile &dexFile) const;
  const std::string GetMemeberProto(const IDexFile &dexFile) const;
  const ResolvedMethodType *GetMethodType(const IDexFile &dexFile) const;
  const std::string GetInvokeKind() const;

 private:
  ResolvedMethodHandleItem() = default;
  ~ResolvedMethodHandleItem() = default;
};

class ResolvedCallSiteIdItem {
 public:
  static const ResolvedCallSiteIdItem *GetInstance(const IDexFile &dexFile, uint32_t index);
  uint32_t GetDataOff() const;
  uint32_t GetMethodHandleIndex(const IDexFile &dexFile) const;
  const ResolvedMethodType *GetMethodType(const IDexFile &dexFile) const;
  const std::string GetProto(const IDexFile &dexFile) const;
  const std::string GetMethodName(const IDexFile &dexFile) const;
  void GetLinkArgument(const IDexFile &dexFile, std::list<std::pair<ValueType, std::string>> &args) const;

 private:
  ResolvedCallSiteIdItem() = default;
  ~ResolvedCallSiteIdItem() = default;
};

class IDexFile {
 public:
  IDexFile() = default;
  virtual ~IDexFile() = default;
  virtual bool Open(const std::string &fileName) = 0;
  virtual const void *GetData() const = 0;
  virtual uint32_t GetFileIdx() const = 0;
  virtual void SetFileIdx(uint32_t fileIdxArg) = 0;
  virtual const uint8_t *GetBaseAddress() const = 0;
  virtual const IDexHeader *GetHeader() const = 0;
  virtual const IDexMapList *GetMapList() const = 0;
  virtual uint32_t GetStringDataOffset(uint32_t index) const = 0;
  virtual uint32_t GetTypeDescriptorIndex(uint32_t index) const = 0;
  virtual const char *GetStringByIndex(uint32_t index) const = 0;
  virtual const char *GetStringByTypeIndex(uint32_t index) const = 0;
  virtual const IDexProtoIdItem *GetProtoIdItem(uint32_t index) const = 0;
  virtual const IDexFieldIdItem *GetFieldIdItem(uint32_t index) const = 0;
  virtual const IDexMethodIdItem *GetMethodIdItem(uint32_t index) const = 0;
  virtual uint32_t GetClassItemsSize() const = 0;
  virtual const IDexClassItem *GetClassItem(uint32_t index) const = 0;
  virtual bool IsNoIndex(uint32_t index) const = 0;
  virtual uint32_t GetTypeIdFromName(const std::string &className) const = 0;
  virtual uint32_t ReadUnsignedLeb128(const uint8_t **pStream) const = 0;
  virtual uint32_t FindClassDefIdx(const std::string &descriptor) const = 0;
  virtual std::unordered_map<std::string, uint32_t> GetDefiningClassNameTypeIdMap() const = 0;

  // ===== DecodeDebug =====
  using DebugNewPositionCallback = int (*)(void *cnxt, uint32_t address, uint32_t lineNum);
  using DebugNewLocalCallback = void (*)(void *cnxt, uint16_t reg, uint32_t startAddress, uint32_t endAddress,
                                         const char *name, const char *descriptor, const char *signature);
  virtual void DecodeDebugLocalInfo(const IDexMethodItem &method, DebugNewLocalCallback newLocalCb) = 0;
  virtual void DecodeDebugPositionInfo(const IDexMethodItem &method, DebugNewPositionCallback newPositionCb)  = 0;

  // ===== Call Site =======
  virtual const ResolvedCallSiteIdItem *GetCallSiteIdItem(uint32_t idx) const = 0;
  virtual const ResolvedMethodHandleItem *GetMethodHandleItem(uint32_t idx) const = 0;
};
}  // namespace maple
#endif  // HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_INTERFACE_H
