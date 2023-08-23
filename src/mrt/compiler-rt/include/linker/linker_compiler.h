/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_MPL_LINKER_COMPILER_H_
#define MAPLE_RUNTIME_MPL_LINKER_COMPILER_H_

#include "muid.h"
#include "address.h"

namespace maplert {
#ifndef LINKER_LAZY_BINDING
#define LINKER_LAZY_BINDING
#endif

#if defined(__ANDROID__)
#define LINKER_RT_CACHE
#define LINKER_DECOUPLE_CACHE
#endif

#ifdef USE_32BIT_REF
constexpr uint32_t kDsoLoadedAddressStart = 0xC0000000;
constexpr uint32_t kDsoLoadedAddressEnd   = 0xF0000000;
#endif

#ifdef USE_32BIT_REF
constexpr uint32_t kNegativeNum = 0x80000000;
constexpr uint32_t kGoldSymbolTableEndFlag = 0X40000000;
#else
constexpr uint64_t kNegativeNum = 0x8000000000000000;
constexpr uint64_t kGoldSymbolTableEndFlag = 0X4000000000000000;
#endif

// We must open LINKER_ADDR_FROM_OFFSET and LINKER_32BIT_REF_FOR_DEF_UNDEF
// at the same time for '.long' and '[sym]-.', such as '.long __cinf_Xxx - .',
// or close them for '.quad __cinf_Xxx'
#ifdef USE_32BIT_REF
#define LINKER_32BIT_REF_FOR_DEF_UNDEF
#define LINKER_ADDR_FROM_OFFSET
#endif

#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF // We use exclusive LINKER_32BIT_REF_FOR_DEF_UNDEF for def/undef USE_32BIT_REF
using LinkerOffsetType = int32_t;
using LinkerVoidType = uint32_t;
#else
using LinkerOffsetType = int64_t;
using LinkerVoidType = uint64_t;
#endif // USE_32BIT_REF

#ifdef USE_32BIT_REF
using VOID_PTR = uint32_t;
using INT_VAL = int32_t;
#else
using VOID_PTR = uint64_t;
using INT_VAL = int64_t;
#endif

static constexpr const char kPreinitModuleClasses[] = "MRT_PreinitModuleClasses";
static constexpr const char kRangeTableFunc[] = "MRT_GetRangeTableBegin";
static constexpr const char kRangeTableEndFunc[] = "MRT_GetRangeTableEnd";
static constexpr const char kMapleStartFunc[] = "MRT_GetMapleStart";
static constexpr const char kMapleEndFunc[] = "MRT_GetMapleEnd";
static constexpr const char kMapleVersionBegin[] = "MRT_GetVersionTabBegin";
static constexpr const char kMapleVersionEnd[] = "MRT_GetVersionTabEnd";
static constexpr const char kMapleCompileStatusBegin[] = "MRT_GetMFileStatusBegin";
static constexpr const char kMapleCompileStatusEnd[] = "MRT_GetMFileStatusEnd";
static constexpr const char kStartHotStrTabBegin[] = "MRT_GetStartHotStrTabBegin";
static constexpr const char kBothHotStrTabBegin[] = "MRT_GetBothHotStrTabBegin";
static constexpr const char kRunHotStrTabBegin[] = "MRT_GetRunHotStrTabBegin";
static constexpr const char kColdStrTabBegin[] = "MRT_GetColdStrTabBegin";
static constexpr const char kColdStrTabEnd[] = "MRT_GetColdStrTabEnd";
static constexpr const char kMetadataFieldStart[] = "MRT_GetMFileROMetadataFieldStart";
static constexpr const char kMetadataFieldEnd[] = "MRT_GetMFileROMetadataFieldEnd";
static constexpr const char kMetadataMethodStart[] = "MRT_GetMFileROMetadataMethodStart";
static constexpr const char kMetadataMethodEnd[] = "MRT_GetMFileROMetadataMethodEnd";
static constexpr const char kMuidTabStart[] = "MRT_GetMFileROMuidtabStart";
static constexpr const char kMuidTabEnd[] = "MRT_GetMFileROMuidtabEnd";
static constexpr const char kBBProfileTabBegin[] = "MRT_GetBBProfileTabBegin";
static constexpr const char kBBProfileTabEnd[] = "MRT_GetBBProfileTabEnd";
static constexpr const char kBBProfileStrTabBegin[] = "MRT_GetBBProfileStrTabBegin";
static constexpr const char kBBProfileStrTabEnd[] = "MRT_GetBBProfileStrTabEnd";

#pragma pack(4)
struct LinkerOffsetKeyTableInfo {
#ifdef USE_32BIT_REF
  int32_t vtableOffsetTable;
  int32_t fieldOffsetTable;
#else
  int64_t vtableOffsetTable;
  int64_t fieldOffsetTable;
#endif
  uint32_t vtableOffsetTableSize;
  uint32_t fieldOffsetTableSize;
};

// vtable offset key table for decoupling
struct LinkerVTableOffsetItem {
  size_t index; // pointer to class metadata
  int32_t methodName;
  int32_t signatureName;
  int32_t fieldIndex;
};

struct LinkerFieldOffsetItem {
  size_t index; // pointer to class metadata
  int32_t fieldName;
  int32_t fieldType;
  int32_t vtableIndex;
};

struct LinkerStaticDecoupleClass {
  size_t callee; // class index
  uint32_t fieldsNum;
  uint32_t methodsNum;
  uint32_t pad;
};

struct LinkerStaticAddrItem {
  VOID_PTR address;
  VOID_PTR index;
  VOID_PTR dcpAddr;
  VOID_PTR classInfo;
};

struct MplStaticFieldItem {
  size_t caller;
  uint32_t fieldName;
  uint32_t fieldType;
  uint32_t fieldIdx;
};

struct MplStaticMethodItem {
  size_t caller;
  uint32_t methodName;
  uint32_t methodSignature;
  uint32_t methodIdx;
};

struct MplStaticAddrTabHead {
  union {
    uintptr_t mplInfo;
    uint64_t pad; // keep 64 bits alignment
  };
  VOID_PTR classSize;
  VOID_PTR tableSize;
  uint64_t magic;
};

struct MplStaticFieldInfo {
  bool isAccess;
  uint64_t fieldAddr;
  uint64_t classInfo;
};

struct MplFieldInfo {
  char *fieldName;
  char *fieldType;
};
#pragma pack()

struct LinkerAddrTableItem {
  LinkerOffsetType addr; // Original method address offset, 64bits

  // We've used AddressFromOffset() as hard code everywhere,
  // so we control using offset(implying 32bit in the case of def/undef)
  // or not in the function, by LINKER_ADDR_FROM_OFFSET.
  void *AddressFromOffset() const {
#ifdef LINKER_ADDR_FROM_OFFSET
    LinkerOffsetType offset = addr;
    char *base = reinterpret_cast<char*>(const_cast<LinkerOffsetType*>(&addr));
    return reinterpret_cast<void*>(base + offset);
#else
    return reinterpret_cast<void*>(static_cast<LinkerVoidType>(addr));
#endif
  }

  void *AddressFromBase(void *baseAddr) const {
    LinkerOffsetType offset = addr;
    char *base = reinterpret_cast<char*>(baseAddr);
    return reinterpret_cast<void*>(base + offset);
  }

  void *Address() const {
    return reinterpret_cast<void*>(static_cast<LinkerVoidType>(addr));
  }
};

// Method undefine table item.
struct LinkerMuidTableItem {
  MUID muid; // MUID, 128bits

  // MUID comparison
  bool operator < (const MUID &tmp) const {
    return muid < tmp;
  }
  bool operator > (const MUID &tmp) const {
    return muid > tmp;
  }
  bool operator == (const MUID &tmp) const {
    return muid == tmp;
  }
};

// Muid sorted index table item for 'LinkerAddrTableItem'.
// Use for SYMBOL/MUID --> ADDRESS.
// See struct 'LinkerAddrTableItem'
struct LinkerMuidIndexTableItem {
  uint32_t index; // Address sorted index, 32bits
};

struct LinkerFuncProfileTableItem {
  uint32_t callTimes; // function call times or bb call times
};

struct LinkerFuncProfDescItem {
  uint64_t hash; // function hash
  // [start,end] indicate the range in profile counter tab
  // which belongs to this function
  uint32_t start;
  uint32_t end;
};

struct LinkerVTableItem {
#ifdef USE_32BIT_REF
  uint32_t index; // Index in undef table, 32bits
#else
  size_t index; // Index in undef table, 64bits
#endif
};

using LinkerITableItem = LinkerVTableItem;

struct LinkerTableItem {
  size_t index;
};

using LinkerSuperClassTableItem = LinkerTableItem;
using LinkerGCRootTableItem = LinkerTableItem;

// offset value table for decoupling
struct LinkerOffsetValItem {
  int32_t offset; // field offset in object layout, or method offset in vtable
};

struct LinkerOffsetValItemLazyLoad {
  reffield_t offsetAddr;
  uint32_t offset; // field offset in object layout, or method offset in vtable
#ifndef USE_32BIT_REF
  int32_t pad;
#endif
};

struct MapleVersionT {
  uint32_t mplMajorVersion;
  uint32_t compilerMinorVersion;
};

// Method info. table item.
struct LinkerInfTableItem {
  uint32_t size; // Method size, 32bits
  int32_t funcNameOffset; // Offset of func name, 32bits
};
} // namespace maplert
#endif // MAPLE_RUNTIME_MPL_LINKER_COMPILER_H_
