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
#include "chelper.h"
#include <sys/time.h>
#include <sys/resource.h>
#include "sizes.h"
#include "chosen.h"
#include "fast_alloc_inline.h"
#include "libs.h"
namespace maplert {
extern "C" void *MRT_CLASSINFO(Ljava_2Flang_2Fref_2FWeakReference_3B);

extern "C" inline void MRT_SetJavaClass(address_t objAddr, address_t klass) {
  // set class.
  StoreRefField(objAddr, kMrtKlassOffset, klass);
  // initialize gc header from prototype.
  MClass *cls = MObject::Cast<MClass>(klass);
  if (auto gcInfo = reinterpret_cast<struct GCTibGCInfo*>(cls->GetGctib())) {
    GCHeaderLVal(objAddr) |= gcInfo->headerProto;
  }

#if RC_TRACE_OBJECT
  if (IsTraceObj(objAddr)) {
    void *callerPc = __builtin_return_address(0);
    LOG2FILE(kLogtypeRcTrace) << "Obj " << std::hex << reinterpret_cast<void*>(objAddr) << std::dec <<
        " RC= " << RefCount(objAddr) << " New at " << std::endl;
    util::PrintPCSymbolToLog(callerPc);
  }
#endif
  // mark finalizable object.
  // check class flag.
  uint32_t classFlag = cls->GetFlag();
  if (modifier::hasFinalizer(classFlag)) {
    SetObjFinalizable(objAddr);
    (*theAllocator).OnFinalizableObjCreated(objAddr);
  }
  if (cls->IsLazyBinding()) {
    (void)LinkerAPI::Instance().LinkClassLazily(cls->AsJclass());
  } else if (modifier::IsColdClass(classFlag)) {
    LinkerAPI::Instance().ResolveColdClassSymbol(cls->AsJclass());
  }
}

extern "C" void MCC_SetJavaClass(address_t objAddr, address_t klass) __attribute__((alias("MRT_SetJavaClass")));
extern "C" inline void MRT_SetJavaArrayClass(address_t objAddr, address_t klass) {
  // set class.
  StoreRefField(objAddr, kMrtKlassOffset, klass);

  // initialize gc header from prototype.
  if (auto gcInfo = reinterpret_cast<struct GCTibGCInfo*>(MObject::Cast<MClass>(klass)->GetGctib())) {
    GCHeaderLVal(objAddr) |= gcInfo->headerProto;
  }

  // array has no finalizer and it is not a reference,
  // so we skip class flag check for array.
  // make the store above visible to other thread
  std::atomic_thread_fence(std::memory_order_release);
}

address_t MCC_Reflect_ThrowInstantiationError() {
  MRT_ThrowNewException("java/lang/InstantiationError", nullptr);
  return 0;
}

extern "C" void MRT_ReflectThrowNegtiveArraySizeException() {
#if UNITTEST
  __MRT_ASSERT(0 && "Array element number is negtive.");
#else
  MRT_ThrowNewExceptionUnw("java/lang/NegativeArraySizeException", nullptr);
  __builtin_unreachable();
#endif
}

extern "C" void MRT_SetThreadPriority(pid_t tid, int32_t priority) {
  errno = 0;
  int ret = ::setpriority(static_cast<int>(PRIO_PROCESS), static_cast<uint32_t>(tid), priority);
  if (UNLIKELY(ret != 0 && errno != 0)) {
    char errMsg[maple::kMaxStrErrorBufLen];
    (void)strerror_r(errno, errMsg, sizeof(errMsg));
    LOG(ERROR) << "::setpriority() in MRT_SetThreadPriority() failed with errno " <<
        errno << ": " << errMsg << maple::endl;
  }
}

#if RC_HOT_OBJECT_DATA_COLLECT
const int kMaxClassNameLen = 256;
const int kMaxProfileClassCount = 5;
std::vector<std::string> LoadTraceClassName() {
#ifdef __ANDROID__
  std::string fileName = "/etc/maple_rc_profile_classes_name.txt";
#else
  std::string fileName = "./maple_rc_profile_classes_name.txt";
#endif
  std::ifstream ifs(fileName, std::ifstream::in);
  std::vector<std::string> classNames;
  if (!ifs.is_open()) {
    LOG(INFO) << "LoadTraceClassName open file failed, no permission or not exist\n";
    return classNames;
  }
  char className[kMaxClassNameLen];
  int profileClassCount = 0;
  // only read kMaxProfileClassCount line of file
  while (ifs.getline(className, kMaxClassNameLen) && (profileClassCount++ < kMaxProfileClassCount)) {
    std::string name(className);
    if (name.size() <= 0) {
      continue;
    }
    // class name should start with 'L' or '['
    if ((name[0] == 'L') || (name[0] == '[')) {
      classNames.push_back(name);
    }
  }
  LOG(INFO) << "LoadTraceClassName success, trace class size = " << classNames.size() << maple::endl;
  ifs.close();
  return classNames;
}

static std::vector<std::string> traceClassesName = LoadTraceClassName();

static bool IsTraceClass(address_t klass) {
  if (klass == 0) {
    return false;
  }
  char *className = MObject::Cast<MClass>(klass)->GetName();
  if (className == nullptr) {
    return false;
  }
  for (auto name : traceClassesName) {
    if (name.compare(0, kMaxClassNameLen, className) == 0) {
      return true;
    }
  }
  return false;
}

static inline void setObjPc(address_t objAddr, unsigned long *pc) {
  for (size_t i = 0; i < kTrackFrameNum; ++i) {
    *reinterpret_cast<unsigned long*>(objAddr + kOffsetTrackPC + kDWordBytes * i) = pc[i];
  }
}

static void DumpObjectBackTrace(address_t obj, address_t klass) {
  if (!IsTraceClass(klass)) {
    return;
  }
  size_t limit = 7; // the stack depth limit of recording new object in debug
  std::vector<uint64_t> stackArray;
  MapleStack::FastRecordCurrentStackPCsByUnwind(stackArray, limit);
  size_t size = stackArray.size();
  unsigned long pc[kTrackFrameNum] = { 0 };
  // only track last kTrackFrameNum frame
  size_t skip = limit - kTrackFrameNum;
  for (size_t i = skip; i < limit && i < size; ++i) {
    pc[i - skip] = static_cast<unsigned long>(stackArray[i]);
  }
  // set track PCs to object header
  setObjPc(obj, pc);
}
#endif // RC_HOT_OBJECT_DATA_COLLECT

#if ALLOC_USE_FAST_PATH
#define MCC_SLOW_NEW_OBJECT MRT_NewObject
#else
#define MCC_SLOW_NEW_OBJECT MCC_NewObj_fixed_class
#endif

extern "C" address_t MCC_SLOW_NEW_OBJECT(address_t klass) {
  MClass *cls = MObject::Cast<MClass>(klass);
  uint32_t mod = cls->GetModifier();
  if (UNLIKELY(modifier::IsAbstract(mod) || modifier::IsInterface(mod))) {
    std::string clsName;
    cls->GetBinaryName(clsName);
    MRT_ThrowNewExceptionUnw("java/lang/InstantiationError", clsName.c_str());
    return 0;
  }
  address_t result = (*theAllocator).NewObj(cls->GetObjectSize());
  if (UNLIKELY(result == 0)) {
    (*theAllocator).OutOfMemory();
  }
  MRT_SetJavaClass(result, klass);
#if RC_HOT_OBJECT_DATA_COLLECT
  DumpObjectBackTrace(result, klass);
#endif
  return result;
}

static inline address_t AllocatorNewArray(size_t elemSize, size_t nElems) {
  if (UNLIKELY(nElems > kAllocArrayMaxSize / elemSize)) {
    return 0;
  }
  size_t size = kJavaArrayContentOffset + elemSize * nElems;
  return (*theAllocator).NewObj(size);
}

extern "C" address_t MRT_ChelperNewobjFlexible(size_t elemSize, size_t len, address_t klass, bool isJNI) {
  address_t result = AllocatorNewArray(elemSize, len);
  if (UNLIKELY(result == 0)) {
    (*theAllocator).OutOfMemory(isJNI);
    return result;
  }
  ArrayLength(result) = static_cast<uint32_t>(len);
  MRT_SetJavaArrayClass(result, klass);
  return result;
}

const unsigned int kClassObjectFlag = 0xF0;

static inline MClass *GetArrayClass(const char *classNameOrClassObj, address_t callerObj, unsigned long classFlag) {
  if (classFlag & kClassObjectFlag) {
    return reinterpret_cast<MClass*>(const_cast<char*>(classNameOrClassObj));
  } else if (classFlag & (~kClassObjectFlag)) {
    return WellKnown::GetWellKnowClassWithFlag(static_cast<uint8_t>(classFlag & (~kClassObjectFlag)),
                                               *MObject::Cast<MClass>(callerObj),
                                               classNameOrClassObj);
  } else {
    return MClass::JniCast(MCC_GetClass(MObject::Cast<MClass>(callerObj)->AsJclass(), classNameOrClassObj));
  }
}

#define MCC_SLOW_NEW_ARRAY MCC_NewObj_flexible_cname
address_t MCC_NewObj_flexible_cname(size_t elemSize,
                                    size_t nElems,
                                    const char *classNameOrClassObj,
                                    address_t callerObj,
                                    unsigned long classFlag) {
  // array length is set in NewArray
  if (nElems > kMrtMaxArrayLength) {
    MRT_ReflectThrowNegtiveArraySizeException();
  }
  MClass *klass = GetArrayClass(classNameOrClassObj, callerObj, classFlag);
  __MRT_ASSERT(klass != nullptr, "MCC_NewObj_flexible_cname klass nullptr");
  address_t result = AllocatorNewArray(elemSize, nElems);
  if (UNLIKELY(result == 0)) {
    (*theAllocator).OutOfMemory();
  }
  ArrayLength(result) = static_cast<uint32_t>(nElems);
  MRT_SetJavaArrayClass(result, klass->AsUintptr());

#if RC_HOT_OBJECT_DATA_COLLECT
  DumpObjectBackTrace(result, klass->AsUintptr());
#endif
  return result;
}

#if ALLOC_USE_FAST_PATH
void MRT_SetFastAlloc(ClassMetadata *classMetadata) {
  __MRT_ASSERT(classMetadata != nullptr, "classMetadata is nullptr in MRT_SetFastAlloc");
  uint16_t flag = classMetadata->flag;
  MClass *cls = reinterpret_cast<MClass*>(classMetadata);
  if (UNLIKELY(modifier::IsFastAllocClass(flag))) {
    LOG(ERROR) << "fast path: init repeated or bad flag " << cls->GetName() << maple::endl;
    classMetadata->flag &= ~modifier::kClassFastAlloc;
    return;
  }
  if (UNLIKELY(modifier::hasFinalizer(flag) ||
               modifier::IsArrayClass(flag) ||
               // potentially, we can enable fast path for lazy/cold classes,
               // but, watch out for race condition
               modifier::IsLazyBindingClass(flag) ||
               modifier::IsColdClass(flag))) {
    return;
  }
  uint32_t mod = cls->GetModifier();
  if (UNLIKELY(modifier::IsAbstract(mod) || modifier::IsInterface(mod))) {
    return;
  }
  size_t objSize = cls->GetObjectSize();
  size_t alignedSize = AllocUtilRndUp(objSize, kAllocAlign);
  if (LIKELY(ROSIMPL_IS_LOCAL_RUN_SIZE((alignedSize + ROSIMPL_HEADER_ALLOC_SIZE)))) {
    classMetadata->sizeInfo.objSize = alignedSize;
    classMetadata->flag |= modifier::kClassFastAlloc;
  }
}
#endif

// try to inline in this .cpp
__attribute__((always_inline))
extern "C" address_t MCC_NewObject(address_t klass) {
  address_t objAddr = MRT_TryNewObject(klass);
  if (LIKELY(objAddr != 0)) {
    return objAddr;
  }
  return MCC_SLOW_NEW_OBJECT(klass);
}

#if ALLOC_USE_FAST_PATH
// change compiler invocation so that this redirection can be removed!
// inline helps when this function is used in this .cpp (e.g., new permanent)
inline address_t MCC_NewObj_fixed_class(address_t klass) {
  return MCC_NewObject(klass);
}
#endif

extern "C" address_t MCC_NewArray8(size_t nElems, address_t klass) {
  address_t objAddr = MRT_TryNewArray<kElemByte>(nElems, klass);
  if (LIKELY(objAddr != 0)) {
    return objAddr;
  }
  const char *param = reinterpret_cast<const char*>(klass);
  return MCC_SLOW_NEW_ARRAY(1, nElems, param, 0, kClassObjectFlag); // 1 is 8bit
}

extern "C" address_t MCC_NewArray16(size_t nElems, address_t klass) {
  address_t objAddr = MRT_TryNewArray<kElemHWord>(nElems, klass);
  if (LIKELY(objAddr != 0)) {
    return objAddr;
  }
  const char *param = reinterpret_cast<const char*>(klass);
  return MCC_SLOW_NEW_ARRAY(2, nElems, param, 0, kClassObjectFlag); // 2 is 16bit
}

extern "C" address_t MCC_NewArray32(size_t nElems, address_t klass) {
  address_t objAddr = MRT_TryNewArray<kElemWord>(nElems, klass);
  if (LIKELY(objAddr != 0)) {
    return objAddr;
  }
  const char *param = reinterpret_cast<const char*>(klass);
  return MCC_SLOW_NEW_ARRAY(4, nElems, param, 0, kClassObjectFlag); // 4 is 32bit
}

extern "C" address_t MCC_NewArray64(size_t nElems, address_t klass) {
  address_t objAddr = MRT_TryNewArray<kElemDWord>(nElems, klass);
  if (LIKELY(objAddr != 0)) {
    return objAddr;
  }
  const char *param = reinterpret_cast<const char*>(klass);
  return MCC_SLOW_NEW_ARRAY(8, nElems, param, 0, kClassObjectFlag); // 8 is 64bit
}

extern "C" address_t MCC_NewArray(size_t nElems, const char *descriptor, address_t callerObj) {
  address_t objAddr = MRT_TryNewArray<kElemWord>(nElems, reinterpret_cast<address_t>(
      MCC_GetClass(MObject::Cast<MClass>(callerObj)->AsJclass(), descriptor)));
  if (LIKELY(objAddr != 0)) {
    return objAddr;
  }
  // this will go the get class path
  return MCC_SLOW_NEW_ARRAY(4, nElems, descriptor, callerObj, 0); // 4 is 32bit
}

#ifdef USE_32BIT_REF
#if !defined(__ANDROID__) && defined(__arm__)
constexpr address_t kKlassHighestBoundary = 0xffffffff;
#else
constexpr address_t kKlassHighestBoundary = 0xdfffffff;
#endif // __ANDROID__
#if __MRT_DEBUG
constexpr address_t kKlassLowestBoundary = 0x80000000;
#endif
#endif //USE_32BIT_REF

static inline void SetPermanentObjectClass(address_t objAddr, address_t klass) {
#ifdef USE_32BIT_REF
#if __MRT_DEBUG
  if ((klass > kKlassHighestBoundary) || (klass < kKlassLowestBoundary)) {
    LOG(FATAL) << "klass is not correct: " << std::hex << klass << std::dec <<
        MObject::Cast<MClass>(klass)->GetName() << maple::endl;
  }
#endif
#endif //USE_32BIT_REF
  // set class.
  StoreRefField(objAddr, kMrtKlassOffset, klass);

  // handle cold class.
  if (MObject::Cast<MClass>(klass)->IsLazyBinding()) {
    (void)LinkerAPI::Instance().LinkClassLazily(reinterpret_cast<jclass>(klass));
  } else if (MObject::Cast<MClass>(klass)->IsColdClass()) {
    LinkerAPI::Instance().ResolveColdClassSymbol(reinterpret_cast<jclass>(klass));
  }
}

static inline void SetPermanentArrayClass(address_t objAddr, address_t klass) {
#ifdef USE_32BIT_REF
  if (UNLIKELY(klass > kKlassHighestBoundary)) {
    LOG(FATAL) << "klass is not correct: " << std::hex << klass << std::dec <<
        MObject::Cast<MClass>(klass)->GetName() << maple::endl;
  }
#endif //USE_32BIT_REF
  // set class.
  StoreRefField(objAddr, kMrtKlassOffset, klass);
}

static inline bool HasChildReference(const MClass *klass) {
  __MRT_ASSERT(klass != nullptr, "kclass is nullptr in HasChildReference");
  const GCTibGCInfo *gcInfo = reinterpret_cast<GCTibGCInfo*>(klass->GetGctib());
  return (gcInfo == nullptr || (gcInfo->headerProto & kHasChildRef) != 0);
}

address_t MCC_NewPermanentObject(address_t klass) {
  // allocate permanent Reference or finalizable objects is not allowed,
  // we fallback to allocate them in heap space. this means the @Permanent
  // annotation for Reference or finalizable objects is ignored.
  const MClass *javaClass = MObject::Cast<MClass>(klass);
  const uint32_t classFlag = javaClass->GetFlag();
  if (UNLIKELY(modifier::IsReferenceClass(classFlag) || modifier::hasFinalizer(classFlag))) {
    return MCC_NewObj_fixed_class(klass);
  }

  // if object has child reference, we allocate it in heap
  // and set it as rc overflowed so that rc operations are ignored.
  if (HasChildReference(javaClass)) {
    address_t result = MCC_NewObj_fixed_class(klass);
    SetRCOverflow(result);
    return result;
  }

  // allocate object in permanent space, without object header.
  address_t result = MRT_AllocFromPerm(javaClass->GetObjectSize());
  if (UNLIKELY(result == 0)) {
    return 0;
  }
  // set class.
  SetPermanentObjectClass(result, klass);

  return result;
}

extern "C" address_t MCC_NewPermObject(address_t klass) __attribute__((alias("MCC_NewPermanentObject")));

address_t MCC_NewPermanentArray(size_t elemSize,
                                size_t nElems,
                                const char *classNameOrClassObj,
                                address_t callerObj,
                                unsigned long classFlag) {
  // determine array class.
  MClass *klass = GetArrayClass(classNameOrClassObj, callerObj, classFlag);
  __MRT_ASSERT(klass != nullptr, "MCC_NewPermanentArray klass nullptr");

  // for object array and length > 0, we allocate it in heap
  // and set it as rc overflowed so that rc operations are ignored.
  if (HasChildReference(klass) && nElems != 0) {
    // this still calls slow path, should switch to fast path
    address_t result = MCC_NewObj_flexible_cname(elemSize, nElems, classNameOrClassObj, callerObj, classFlag);
    SetRCOverflow(result);
    return result;
  }

  // check length.
  if (UNLIKELY(nElems > kMrtMaxArrayLength)) {
    MRT_ReflectThrowNegtiveArraySizeException();
  }

  // allocate array in permanent space, without object header.
  const size_t totalSize = kJavaArrayContentOffset + elemSize * nElems;
  address_t result = MRT_AllocFromPerm(totalSize);
  if (UNLIKELY(result == 0)) {
    return 0;
  }
  // set class.
  SetPermanentArrayClass(result, reinterpret_cast<address_t>(klass));

  // set array length.
  ArrayLength(result) = static_cast<uint32_t>(nElems);

  return result;
}

template<ArrayElemSize elemSizeExp>
static address_t MRT_NewPermArray(size_t nElems, address_t klass) {
  // objects in permanent space cannot have child references, use normal heap
  if (HasChildReference(MObject::Cast<MClass>(klass)) && nElems != 0) {
    address_t objAddr = MRT_TryNewArray<elemSizeExp>(nElems, klass);
    if (UNLIKELY(objAddr == 0)) {
      const char *param = reinterpret_cast<const char*>(klass);
      objAddr = MCC_SLOW_NEW_ARRAY(1 << elemSizeExp, nElems, param, 0, kClassObjectFlag);
    }
    SetRCOverflow(objAddr);
    return objAddr;
  }

  if (UNLIKELY(nElems > kMrtMaxArrayLength)) {
    MRT_ReflectThrowNegtiveArraySizeException();
  }
  size_t objSize = kJavaArrayContentOffset + (nElems << elemSizeExp);
  address_t objAddr = MRT_AllocFromPerm(objSize);
  if (UNLIKELY(objAddr == 0)) {
    return 0;
  }
  // permanent allocator does not properly throw OOM, it will just abort
  SetPermanentArrayClass(objAddr, klass);
  ArrayLength(objAddr) = static_cast<uint32_t>(nElems);
  return objAddr;
}

extern "C" address_t MCC_NewPermArray8(size_t nElems, address_t klass) {
  return MRT_NewPermArray<kElemByte>(nElems, klass);
}

extern "C" address_t MCC_NewPermArray16(size_t nElems, address_t klass) {
  return MRT_NewPermArray<kElemHWord>(nElems, klass);
}

extern "C" address_t MCC_NewPermArray32(size_t nElems, address_t klass) {
  return MRT_NewPermArray<kElemWord>(nElems, klass);
}

extern "C" address_t MCC_NewPermArray64(size_t nElems, address_t klass) {
  return MRT_NewPermArray<kElemDWord>(nElems, klass);
}

address_t MCC_NewPermArray(size_t nElems, const char *descriptor, address_t callerObj) {
  return MRT_NewPermArray<kElemWord>(nElems, reinterpret_cast<address_t>(
      MCC_GetClass(MObject::Cast<MClass>(callerObj)->AsJclass(), descriptor)));
}

  // set object rc overflow to ignore rc operation
extern "C" void MRT_SetObjectPermanent(address_t objAddr) {
  if (!IS_HEAP_OBJ(objAddr)) {
    return;
  }
  SetRCOverflow(objAddr);
}

extern "C" void MCC_SetObjectPermanent(address_t objAddr) __attribute__((alias("MRT_SetObjectPermanent")));

extern "C" void MRT_CheckRefCount(address_t objAddr, uint32_t index) {
  if (!IS_HEAP_OBJ(objAddr)) {
    return;
  }

  uint32_t rc = RefCount(objAddr);
  if (rc == 0) {
#ifdef __ANDROID__
    LOG(ERROR) << "Obj = 0x" << std::hex << objAddr << " RCHeader = 0x" << RCHeader(objAddr) <<
        " GCHeader = 0x" << GCHeader(objAddr) <<
        " LockWord = 0x" << *(reinterpret_cast<uint32_t*>(objAddr + kLockWordOffset)) <<
        " Index = " << std::dec << index << ", RC Fatal Error is detected." << std::endl;
#else
    std::cout << "Obj = 0x" << std::hex << objAddr << " RCHeader = 0x" << RCHeader(objAddr) <<
        " GCHeader = 0x" << GCHeader(objAddr) <<
        " LockWord = 0x" << *(reinterpret_cast<uint32_t*>(objAddr + kLockWordOffset)) <<
        " Index = " << std::dec << index << ", RC Fatal Error is detected." << std::endl;
#endif
    HandleRCError(objAddr);
  }
}

extern "C" void MCC_CheckRefCount(address_t objAddr, uint32_t index) __attribute__((alias("MRT_CheckRefCount")));
} // namespace maplert
