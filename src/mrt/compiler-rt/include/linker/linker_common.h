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
#ifndef MAPLE_RUNTIME_MPL_LINKER_COMMON_H_
#define MAPLE_RUNTIME_MPL_LINKER_COMMON_H_

#include <link.h>
#include <assert.h>

#include "mrt_api_common.h"
#include "jni.h"
#include "linker_compiler.h"
#include "object_type.h"
#include "metadata_layout.h"
#ifdef LINKER_DECOUPLE
#include "decouple/linker_decouple_common.h"
#endif

#ifdef __cplusplus
#include <unordered_map>
#include <tuple>
#include <set>
#include <map>
#include <vector>
#include <unordered_set>
namespace maplert {
// For un|define tables' address.
using PtrFuncType = void *(*)();

#define LINKER_INSTALL_STATE (maple::MplCacheFirstFile::GetInstallStat())

#ifdef LINKER_RT_CACHE
enum class LinkerCacheType : uint8_t {
  kLinkerCacheDataUndef = 0,   // data undef symbol
  kLinkerCacheDataDef = 1,     // data def symbol
  kLinkerCacheMethodUndef = 2, // method symbol
  kLinkerCacheMethodDef = 3,   // method symbol
  kLinkerCacheVTable = 4,      // vtable
  kLinkerCacheCTable = 5,      // vtable offset table
  kLinkerCacheFieldOffset = 6, // field offset
  kLinkerCacheFieldTable = 7,  // field offset table
  kLinkerCacheITable = 8,      // itable
  kLinkerCacheStaticTable = 9, // static table
  kLinkerCacheLazy = 10        // lazy cache
};

static constexpr char kLinkerSystemCachePath[] = "/data/system";
static constexpr char kLinkerRootCachePath[] = "/data/dalvik-cache";
static constexpr char kLinkerCacheFold[] = "mpl-lnk";
static constexpr unsigned int kLinkerIndexMax = 0xFFFFFFFF;
static constexpr int kLinkerClHashFlagValue = 0;
static constexpr unsigned int kLinkerClHashFlagIndex = 0xFFFF;
// Crash type(count) kLinkerServiceCrashRecoveryMygote(2) for mygote recovery.
static constexpr int kLinkerServiceCrashRecoveryMygote = 2;
// Crash type(count) kLinkerServiceCrashRecoverySystemServer(3) for sysvr recovery.
static constexpr int kLinkerServiceCrashRecoverySystemServer = 3;

// Method undefine cache table item.
class LinkerCacheTableItem {
 public:
  LinkerCacheTableItem() : addrIndex(0), soIndex(0), filled(0), state(kValid) {}

  LinkerCacheTableItem(const LinkerCacheTableItem &item) {
    addrIndex = item.addrIndex;
    soIndex = item.soIndex;
    filled = item.filled;
    state = item.state;
  }

  LinkerCacheTableItem &operator=(const LinkerCacheTableItem &item) {
    this->addrIndex = item.addrIndex;
    this->soIndex = item.soIndex;
    this->filled = item.filled;
    this->state = item.state;
    return *this;
  }
  LinkerCacheTableItem(uint32_t aidx, uint16_t soidx) : addrIndex(aidx), soIndex(soidx) {
    filled = 0;
    state = kValid;
  }
  ~LinkerCacheTableItem() = default;
  uint32_t AddrId() const {
    return addrIndex;
  }
  uint16_t SoId() const {
    return soIndex;
  }

  void SetFilled() {
    filled = 1;
  }
  bool Filled() const {
    return filled == 1;
  }
  bool LazyInvalidName() const {
    return state == kLazyInvalid;
  }
  bool InvalidName() const {
    return state == kInvalid;
  }
  bool Valid() const {
    return state == kValid;
  }

  LinkerCacheTableItem &SetIds(size_t addrId, uint16_t soid) {
    addrIndex = static_cast<uint32_t>(addrId);
    soIndex = soid;
    state = kValid;
    return *this;
  }
  LinkerCacheTableItem &SetLazyInvalidSoId(uint16_t soidx) {
    soIndex = soidx;
    state = kLazyInvalid;
    return *this;
  }
  LinkerCacheTableItem &SetInvalidSoId(uint16_t soidx) {
    soIndex = soidx;
    state = kInvalid;
    return *this;
  }
 private:
  uint32_t addrIndex; // Index, the same order as item in undef|def table
  uint16_t soIndex; // index for maple so
  uint8_t filled; // If this item was handled before.
  uint8_t state; // if maple so name valid

  enum SoIdState : uint8_t {
    kValid = 0,
    kInvalid = 1,
    kLazyInvalid = 2,
  };
};

using MplCacheMapT = std::unordered_map<uint32_t, LinkerCacheTableItem>;

struct LinkerCacheRep {
  MplCacheMapT methodUndefCacheMap;
  int64_t methodUndefCacheSize = -1;
  MplCacheMapT methodDefCacheMap;
  int64_t methodDefCacheSize = -1;
  std::string methodUndefCachePath;
  std::string methodDefCachePath;
  MplCacheMapT dataUndefCacheMap;
  int64_t dataUndefCacheSize = -1;
  MplCacheMapT dataDefCacheMap;
  int64_t dataDefCacheSize = -1;
  std::string dataUndefCachePath;
  std::string dataDefCachePath;
  std::string lazyCachePath;
};
#endif // LINKER_RT_CACHE

// Refer to MUIDReplacement::GenRangeTable() for sequence.
enum SectTabIndex {
  kRange = 0,
  kDecouple,
  kVTable,
  kITable,
  kVTabOffset,
  kFieldOffset,
  kValueOffset,
  kLocalCinfo,
  kDataConstStr,
  kDataSuperClass,
  kDataGcRoot,
  kTabClassMetadata,
  kClassMetadataBucket,
  kJavaText,
  kJavaJni,
  kJavaJniFunc,
  kMethodDef,
  kMethodDefOrig,
  kMethodInfo,
  kMethodUndef,
  kDataDef,
  kDataDefOrig,
  kDataUndef,
  kMethodDefMuid,
  kMethodUndefMuid,
  kDataDefMuid,
  kDataUndefMuid,
  kMethodMuidIndex,
  kMethodProfile,
  kDataSection,
  kStaticDecoupleKey,
  kStaticDecoupleValue,
  kBssSection,
  kLinkerHashSo,
  kArrayClassCacheIndex,
  kArrayClassCacheNameIndex,
  kFuncIRProfDesc,
  kFuncIRProfCounter,
  kMaxTableIndexCount
};

// Start/end address of table
enum SectTabPairIndex {
  kTable1stIndex,
  kTable2ndIndex,
  kTable2ndDimMaxCount
};

// State of address, for lazy binding.
enum BindingState {
  kBindingStateUnresolved   = 0, // Represents all kinds of unresolved state.
  kBindingStateCinfUndef    = 1,
  kBindingStateCinfDef      = 2,
  kBindingStateDataUndef    = 3,
  kBindingStateDataDef      = 4,
  kBindingStateMethodUndef  = 5,
  kBindingStateMethodDef    = 6,
  kBindingStateMax          = 4 * 1024, // a page
  kBindingStateResolved // Already resolved if great than Max.
};

// Type for searching address.
enum AddressRangeType {
  kTypeWhole  = 0, // The range is the whole MFile.
  kTypeText   = 1, // The range is Java Text sections
  kTypeClass  = 2, // The range is Class Metadata sections.
  kTypeData   = 3  // The range is Data section
};

// State of loading process.
enum LoadStateType {
  kLoadStateNone = 0, // The state of no loading or loading finished.
  kLoadStateBoot = 1, // The state of loading in boot(mygote).
  kLoadStateSS   = 2, // The state of loading system_server.
  kLoadStateApk  = 3  // The state of loading APK.
};

// State of app loading.
enum AppLoadState {
  kAppLoadNone          = 0, // Not loading APK for App.
  kAppLoadBaseOnlyReady = 1, // Ready to load the base APK
  kAppLoadBaseOnly      = 2, // Loading the base APK and successive
  kAppLoadBaseAndOthers = 3  // Start to load other APKs
};

enum MFileInfoSource {
  kFromPC = 0,
  kFromAddr,
  kFromMeta,
  kFromHandle,
  kFromName,
  kFromClass,
  kFromClassLoader
};

using PtrMatrixType = void *(*)[kTable2ndDimMaxCount];

struct FuncProfInfo {
  uint32_t callTimes;
  uint8_t layoutType;
  std::string funcName;
  FuncProfInfo(uint32_t callTimes, uint8_t layoutType) : callTimes(callTimes), layoutType(layoutType) {}
  FuncProfInfo(uint32_t callTimes, uint8_t layoutType, std::string funcName)
      : callTimes(callTimes), layoutType(layoutType), funcName(std::move(funcName)) {}
};

struct FuncIRProfInfo {
  std::string funcName;
  uint64_t hash;
  uint32_t start; // indicate the start index in counter Table
  uint32_t end;
  FuncIRProfInfo(std::string funcName, uint64_t hash, uint32_t start, uint32_t end)
      : funcName(std::move(funcName)), hash(hash), start(start), end(end) {}
};

struct MFileIRProfInfo {
  std::vector<FuncIRProfInfo> descTab;
  std::vector<uint32_t> counterTab;
};

struct LinkerLocInfo {
  std::string path; // Path name of maple library
  void *addr = nullptr; // Method address
  uint32_t size = 0; // Method size
  std::string sym; // Method symbol pointer
};

using ClassLoaderListT = std::vector<jobject>;
using ClassLoaderListItT = std::vector<jobject>::iterator;
using ClassLoaderListRevItT = std::vector<jobject>::reverse_iterator;

enum MFileInfoFlags {
  // LinkerMFileInfo flags
  kIsBoot = 0,
  kIsLazy,
  kIsRelMethodOnce,
  kIsRelDataOnce,
  kIsMethodDefResolved,
  kIsDataDefResolved,
  kIsVTabResolved,
  kIsITabResolved,
  kIsVTabOffsetResolved,
  kIsFieldOffsetResolved,
  kIsSuperClassResolved,
  kIsGCRootListResolved,
  kIsMethodUndefHasNotResolved,
  kIsMethodDefHasNotResolved,
  kIsDataUndefHasNotResolved,
  kIsDataDefHasNotResolved,
  kIsMethodRelocated,
  kIsDataRelocated,
  kIsMethodUndefCacheValid,
  kIsMethodCacheValid,
  kIsDataUndefCacheValid,
  kIsDataCacheValid,
  // DecoupleMFileInfo flags
  kIsRelDecoupledClassOnce,
  kIsDecoupledVTabResolved,
  kIsDecoupledFieldOffsetResolved,
  kIsFieldOffsetTabResolved,
  kIsDecoupledStaticAddrTabResolved,
  kIsFieldOffsetTrashCacheFileRemoved,
  kIsFieldTableTrashCacheFileRemoved,
  kIsStaticTrashCacheFileRemoved,
  kIsVTabTrashCacheFileRemoved,
  kIsCTabTrashCacheFileRemoved,
  kIsITabTrashCacheFileRemoved,
  kMFileInfoFlagMaxIndex
};

// each interval part for SectTabIndex Item
static constexpr size_t kTableSizeInterval[] = {
    1, 1, sizeof(LinkerVTableItem), sizeof(LinkerITableItem), sizeof(LinkerVTableOffsetItem),
    sizeof(LinkerFieldOffsetItem), sizeof(int32_t), sizeof(size_t), sizeof(void*), sizeof(LinkerSuperClassTableItem),
    sizeof(LinkerGCRootTableItem), 1, 1, 1, 1, 1, sizeof(LinkerAddrTableItem), sizeof(LinkerAddrTableItem),
    sizeof(LinkerInfTableItem), sizeof(LinkerAddrTableItem), sizeof(LinkerAddrTableItem), sizeof(LinkerAddrTableItem),
    sizeof(LinkerAddrTableItem), sizeof(LinkerMuidTableItem), sizeof(LinkerMuidTableItem), sizeof(LinkerMuidTableItem),
    sizeof(LinkerMuidTableItem), sizeof(uint32_t), sizeof(LinkerFuncProfileTableItem), 1,
    sizeof(LinkerStaticDecoupleClass), 1, 1, 1, 1, 1, sizeof(LinkerFuncProfDescItem),
    sizeof(LinkerFuncProfileTableItem), 1
};
// each offset part for SectTabIndex Item
static constexpr size_t kTableSizeOffset[] = {
    0, 0, 0, 0, 0, 0, sizeof(LinkerOffsetKeyTableInfo) / sizeof(int32_t), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sizeof(MplStaticAddrTabHead) / sizeof(LinkerStaticAddrItem), 0, 0, 0, 0, 0, 0, 0
};
using MplFuncProfile = std::unordered_map<uint32_t, FuncProfInfo>;
class LinkerMFileInfo {
 public:
  LinkerMFileInfo() {
    classLoader = nullptr;
    startHotStrTab = nullptr;
    bothHotStrTab = nullptr;
    runHotStrTab = nullptr;
    coldStrTab = nullptr;
    coldStrTabEnd = nullptr;
    rometadataFieldStart = nullptr;
    rometadataFieldEnd = nullptr;
    rometadataMethodStart = nullptr;
    rometadataMethodEnd = nullptr;
    romuidtabStart = nullptr;
    romuidtabEnd = nullptr;
  }
  virtual ~LinkerMFileInfo() {}
  void *elfBase = nullptr; // ELF base address.
  void *elfStart = nullptr; // ELF tables start address.
  void *elfEnd = nullptr; // ELF tables ending address.
  std::string name; // Maple file path name.
  void *handle = nullptr; // Maple file open handle.
  MUID hash; // Hash code of .so.
  MUID hashOfDecouple; // Hash code for decouple of .so.
  uint32_t rangeTabSize = 0;
  uint64_t decoupleStaticLevel = 0;

  PtrMatrixType tableAddr = 0;

  MplFuncProfile funProfileMap;
#ifdef LINKER_RT_CACHE
  LinkerCacheRep rep;
#endif

  jobject classLoader;
  ClassLoaderListT clList;

  char *startHotStrTab;
  char *bothHotStrTab;
  char *runHotStrTab;
  char *coldStrTab;
  char *coldStrTabEnd;

  void *rometadataFieldStart;
  void *rometadataFieldEnd;
  void *rometadataMethodStart;
  void *rometadataMethodEnd;

  void *romuidtabStart;
  void *romuidtabEnd;

  MUID vtbCLHash;
  MUID ctbCLHash;
  MUID fosCLHash;
  MUID ftbCLHash;
  MUID staticCLHash;

#if defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
  int methodCachingNum = 0;
  std::string methodCachingStr;
  std::mutex methodCachingLock;
#endif // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)

  bool BelongsToApp();
  void ReleaseReadOnlyMemory();
#ifdef LINKER_DECOUPLE
  virtual uint64_t GetDecoupleLevel() const = 0;
  virtual void SetDecoupleLevel(uint64_t level) = 0;
#endif
  template<typename Type = void*, typename Convert = Type>
  inline Type GetTblBegin(SectTabIndex index) {
    assert(tableAddr != nullptr);
    index = ((index == kMethodDefOrig || index == kDataDefOrig) && !flag[kIsLazy]) ?
        static_cast<SectTabIndex>(static_cast<int>(index) - 1) : index;
    return static_cast<Convert>(reinterpret_cast<Type>(tableAddr[index][kTable1stIndex]));
  }
  template<typename Type = void*, typename Convert = Type>
  inline Type GetTblEnd(SectTabIndex index) {
    assert(tableAddr != nullptr);
    index = ((index == kMethodDefOrig || index == kDataDefOrig) && !flag[kIsLazy]) ?
        static_cast<SectTabIndex>(static_cast<int>(index) - 1) : index;
    return static_cast<Convert>(reinterpret_cast<Type>(tableAddr[index][kTable2ndIndex]));
  }
  template<typename Type = size_t>
  inline Type GetTblSize(SectTabIndex index) {
    assert(tableAddr != nullptr);
    size_t begin = reinterpret_cast<size_t>(tableAddr[index][kTable1stIndex]);
    size_t end = reinterpret_cast<size_t>(tableAddr[index][kTable2ndIndex]);
    size_t result = (end - begin) / kTableSizeInterval[index];
    size_t offset = kTableSizeOffset[index];
    size_t size = (begin == 0 || end == 0 || result <= offset) ? 0 : (result - offset);
    return static_cast<Type>(size);
  }
  inline bool IsFlag(MFileInfoFlags flagIndex) const {
    return flag[flagIndex];
  }
  inline void SetFlag(MFileInfoFlags flagIndex, bool isTrue) {
    flag[flagIndex] = isTrue;
  }
 private:
  bool flag[kMFileInfoFlagMaxIndex] = { false };
};

class LinkerRef : public DataRef {
 public:
  LinkerRef() = default;
  template<typename T>
  explicit LinkerRef(T ref) {
    SetDataRef<T>(ref);
  }
  LinkerRef(const LinkerRef&) = default;
  ~LinkerRef() = default;
  inline bool IsEmpty() const {
    return refVal == 0;
  }
  inline bool IsVTabIndex() const { // 1 marks a conflict.
    return ((refVal & kNegativeNum) == 0) && refVal != 1 && refVal != 0;
  }
  inline bool IsITabIndex() const { // 1 marks a conflict.
    return (refVal & kNegativeNum) == 0 && refVal != 1;
  }
  inline bool IsIndex() const {
    return (refVal & kFromIndexMask) != 0;
  }
  inline bool IsTabUndef() const {
    constexpr uintptr_t kTableUndefBitMask = 0x2;
    return (refVal & kTableUndefBitMask) == 0;
  }
  inline bool IsFromUndef() const {
    return (refVal & kLinkerRefUnDef) != 0;
  }
  inline size_t GetTabIndex() const {
    return static_cast<size_t>((refVal >> 2) - 1); // 2 bits as flag, real index is start with 3 bits;
  }
  inline size_t GetIndex() const {
#if defined(__aarch64__)
    constexpr uintptr_t kAddressMask = 0x0fffffffffffffff;
    return static_cast<size_t>(refVal & kAddressMask);
#elif defined(__arm__)
    return static_cast<size_t>(refVal >> 2); // low 2 bit as flag
#else
    return 0;
#endif
  }
 protected:
#if defined(__aarch64__)
  enum LinkerRefFormat {
    kLinkerRefAddress   = 0ul, // must be 0
    kLinkerRefDef       = 1ul << 61, // 61 bit as Def
    kLinkerRefUnDef     = 1ul << 62, // 62 bit as Undef
    kFromIndexMask      = kLinkerRefDef | kLinkerRefUnDef
  };
#elif defined(__arm__)
  enum LinkerRefFormat {
    kLinkerRefAddress   = 0ul, // must be 0
    kLinkerRefDef       = 1ul, // def
    kLinkerRefUnDef     = 2ul, // undef
    kFromIndexMask      = kLinkerRefDef | kLinkerRefUnDef
  };
#endif
};

template<typename T>
class Slice {
 public:
  Slice() : base(nullptr), num(0) {}
  Slice(T *b, size_t n) : base(b), num(n) {}
  ~Slice() {
    base = nullptr;
    num = 0;
  }
  Slice(const Slice&) = default;
  Slice &operator=(const Slice&) = default;
  T *Data() {
    return base;
  }
  const T *Data() const {
    return base;
  }
  size_t Size() const {
    return num;
  }
  // Required: n < num.
  T &operator[](size_t n) {
    return base[n];
  }
  const T &operator[](size_t n) const {
    return base[n];
  }
  // Required: num >= n.
  Slice &operator+=(size_t n) {
    base += n;
    num -= n;
    return *this;
  }
  bool Empty() const {
    return (base == nullptr || num == 0);
  }
  void Clear() {
    base = nullptr;
    num = 0;
  }

 private:
  T *base;
  size_t num;
};

using AddrSlice = Slice<LinkerAddrTableItem>;
using MuidSlice = Slice<LinkerMuidTableItem>;
using MuidIndexSlice = Slice<LinkerMuidIndexTableItem>;
using BufferSlice = Slice<char>;
using IndexSlice = Slice<LinkerTableItem>;
using VTableSlice = Slice<LinkerVTableItem>;
using ITableSlice = VTableSlice;
using InfTableSlice = Slice<LinkerInfTableItem>;
}
#endif // __cplusplus
#endif // MAPLE_RUNTIME_MPL_LINKER_COMMON_H_
