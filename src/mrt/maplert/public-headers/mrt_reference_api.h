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
#ifndef MAPLE_LIB_CORE_MPL_REFERENCE_H_
#define MAPLE_LIB_CORE_MPL_REFERENCE_H_

#include <functional>

// mrt/libmrtbase/include/gc_config.h
#include <vector>
#include <map>
#include "mrt_api_common.h"
#include "jni.h"
#include "gc_config.h"
#include "gc_callback.h"
#include "gc_reason.h"
#include "gc_roots.h"

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

// cycle pattern valid flag
const int kCycleValid = 0;
const int kCycleNotValid = 1;
const int kCyclePermernant = 2;
// cycle pattern node flag
const unsigned int kCycleNodeSubClass = 0x1;
// cycle pattern edge flag
const unsigned int kCycleEdgeSkipMatch = 0x1;

MRT_EXPORT void MRT_IncRef(address_t obj);
MRT_EXPORT void MRT_DecRef(address_t obj);
MRT_EXPORT void MRT_IncDecRef(address_t incObj, address_t decObj);
MRT_EXPORT void MRT_DecRefUnsync(address_t obj);
MRT_EXPORT void MRT_IncResurrectWeak(address_t obj);

MRT_EXPORT void MRT_ReleaseObj(address_t obj);
MRT_EXPORT void MRT_CollectWeakObj(address_t obj);
MRT_EXPORT address_t MRT_IncRefNaiveRCFast(address_t obj);
MRT_EXPORT address_t MRT_DecRefNaiveRCFast(address_t obj);
MRT_EXPORT void MRT_IncDecRefNaiveRCFast(address_t incAddr, address_t decAddr);
// encoding the cycle pattern
MRT_EXPORT void MRT_GetCyclePattern(std::ostream& os);
MRT_EXPORT std::ostream *MRT_GetCycleLogFile();

// collector specific JNI interface
// Unsafe
MRT_EXPORT bool MRT_UnsafeCompareAndSwapObject(address_t obj, ssize_t offset,
                                               address_t expectedValue, address_t newValue);
MRT_EXPORT address_t MRT_UnsafeGetObjectVolatile(address_t obj, ssize_t offset);
MRT_EXPORT address_t MRT_UnsafeGetObject(address_t obj, ssize_t offset);
MRT_EXPORT void MRT_UnsafePutObject(address_t obj, ssize_t offset, address_t newValue);
MRT_EXPORT void MRT_UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue);
MRT_EXPORT void MRT_UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue);

// weak global reference decoding barrier
MRT_EXPORT void MRT_WeakRefGetBarrier(address_t referent);

// get the reference count of an object. only for external usage.
MRT_EXPORT uint32_t MRT_RefCount(address_t obj);

MRT_EXPORT bool MRT_CheckHeapObj(uintptr_t obj);
MRT_EXPORT bool MRT_IsGarbage(address_t obj);
MRT_EXPORT bool MRT_IsValidOffHeapObject(jobject obj);

MRT_EXPORT bool MRT_EnterSaferegion(bool rememberLastJavaFrame = true);
MRT_EXPORT bool MRT_LeaveSaferegion();

MRT_EXPORT address_t MRT_GetHeapLowerBound();
MRT_EXPORT address_t MRT_GetHeapUpperBound();
MRT_EXPORT bool MRT_IsValidObjAddr(address_t obj);

// Keep these consistent with compiler-rt/include/cinterface.h
struct VMHeapParam {
  size_t heapStartSize;
  size_t heapSize;
  size_t heapGrowthLimit;
  size_t heapMinFree;
  size_t heapMaxFree;
  float heapTargetUtilization;
  bool ignoreMaxFootprint;
  bool gcOnly = false;
  bool enableGCLog = false;
  bool isZygote = false;
};

MRT_EXPORT bool MRT_GCInitGlobal(const VMHeapParam &vmHeapParam);
MRT_EXPORT bool MRT_GCFiniGlobal();
MRT_EXPORT bool MRT_GCInitThreadLocal(bool isMain);
MRT_EXPORT bool MRT_GCFiniThreadLocal();
MRT_EXPORT void MRT_GCStart();

// fork handlers.
MRT_EXPORT void MRT_GCPreFork();
MRT_EXPORT void MRT_GCPostForkChild(bool isSystem);
MRT_EXPORT void MRT_GCPostForkCommon(bool isZygote);
MRT_EXPORT void MRT_ForkInGC(bool flag);
MRT_EXPORT void MRT_GCLogPostFork();
MRT_EXPORT void MRT_RegisterNativeAllocation(size_t byte);
MRT_EXPORT void MRT_RegisterNativeFree(size_t byte);
MRT_EXPORT void MRT_NotifyNativeAllocation();

// returns number of GC occurred since this function was called last time and
// stop-the-world time maximum in ms. both counts are reset after the call
MRT_EXPORT void MRT_GetGcCounts(size_t &gcCount, uint64_t &maxGcMs);

// returns info for memory leak identified by backup tracing
// average leak and peak leak memory size in Bytes
// both numbers are reset are the call
MRT_EXPORT void MRT_GetMemLeak(size_t &avgLeak, size_t &peakLeak);

// returns the memory utilization rate (higher the better)
// and allocation
MRT_EXPORT void MRT_GetMemAlloc(float &util, size_t &abnormalCount);

// returns number of RC abnormal such as rc counts
// increased from zero to one
MRT_EXPORT void MRT_GetRCParam(size_t &abnormalCount);

inline unsigned long MRT_GetGCUsedHeapMemoryTotal() {
  return 0;
};

MRT_EXPORT void     MRT_ResetHeapStats();
MRT_EXPORT size_t   MRT_AllocSize();
MRT_EXPORT size_t   MRT_AllocCount();
MRT_EXPORT size_t   MRT_FreeSize();
MRT_EXPORT size_t   MRT_FreeCount();
MRT_EXPORT size_t   MRT_TotalMemory();
// returns the max memory usage
MRT_EXPORT size_t   MRT_MaxMemory();
// returns the free memory within the limit
MRT_EXPORT size_t   MRT_FreeMemory();
MRT_EXPORT void     MRT_SetHeapProfile(int hp);
// return all live instances of the given class
MRT_EXPORT void     MRT_GetInstances(jclass klass, bool includeAssignable,
    size_t max_count, std::vector<jobject> &instances);

MRT_EXPORT void  MRT_DebugCleanup();
MRT_EXPORT void  MRT_RegisterGCRoots(address_t *gcroots[], size_t len);
MRT_EXPORT void  MRT_RegisterRCCheckAddr(uint64_t *addr);

MRT_EXPORT void  MRT_SetReferenceProcessMode(bool immediate);
MRT_EXPORT void *MRT_ProcessReferences(void *args);
MRT_EXPORT void  MRT_StopProcessReferences(bool doFinalizeOnStop = false);
MRT_EXPORT void  MRT_WaitProcessReferencesStopped();
MRT_EXPORT void  MRT_WaitProcessReferencesStarted();

MRT_EXPORT void  MRT_WaitGCStopped();
MRT_EXPORT void  MRT_CheckSaferegion(bool expect, const char *msg);

// dump RC and GC information into stream os
MRT_EXPORT void MRT_DumpRCAndGCPerformanceInfo(std::ostream &os);

// Enable/disable GC triggers
MRT_EXPORT void MRT_TriggerGC(maplert::GCReason reason);
MRT_EXPORT bool MRT_IsGcThread();
MRT_EXPORT bool MRT_IsNaiveRCCollector();

MRT_EXPORT bool MRT_IsValidObjectAddress(address_t obj);
MRT_EXPORT void MRT_VisitAllocatedObjects(maple::rootObjectFunc func);

// Alloc tracking
MRT_EXPORT void MRT_SetAllocRecordingCallback(std::function<void(address_t, size_t)> callback);

// Trim memory
MRT_EXPORT void MRT_Trim(bool aggressive);
MRT_EXPORT void MRT_RequestTrim();

MRT_EXPORT void MRT_SetHwBlobClass(jclass cls);
MRT_EXPORT void MRT_SetSurfaceControlClass(jclass cls);
MRT_EXPORT void MRT_SetBinderProxyClass(jclass cls);

// Udpate process state
MRT_EXPORT void MRT_UpdateProcessState(ProcessState processState, bool isSystemServer);

// cycle pattern
MRT_EXPORT void MRT_DumpDynamicCyclePatterns(std::ostream &os, size_t limit);
MRT_EXPORT void MRT_SendCyclePatternJob(std::function<void()> job);
MRT_EXPORT void MRT_SetPeriodicSaveCpJob(std::function<void()> job);
MRT_EXPORT void MRT_SetPeriodicLearnCpJob(std::function<void()> job);
MRT_EXPORT bool MRT_IsCyclePatternUpdated();

// Set a callback function which is called after GC finished, but before
// starting the world.  Useful for performing cleaning up after a GC.
MRT_EXPORT void MRT_DumpStaticField(std::ostream &os);
MRT_EXPORT void MRT_logRefqueuesSize();

// MRT_PreWriteRefField should be called before we directly write a reference field
// of an java heap object but not use write barrier such as MRT_WriteRefField().
// This is required for concurrent marking.
MRT_EXPORT void MRT_PreWriteRefField(address_t obj);
MRT_EXPORT size_t MRT_GetNativeAllocBytes();
MRT_EXPORT void   MRT_SetNativeAllocBytes(size_t size);

MRT_EXPORT address_t MRT_LoadVolatileField(address_t obj, address_t *fieldAddr);
MRT_EXPORT address_t MRT_LoadRefFieldCommon(address_t obj, address_t *fieldAddr);
MRT_EXPORT void MRT_ClassInstanceNum(std::map<std::string, long> &objNameCntMp);

// reference api
MRT_EXPORT address_t MRT_ReferenceGetReferent(address_t javaThis);
MRT_EXPORT void    MRT_ReferenceClearReferent(address_t javaThis);
MRT_EXPORT void    MRT_RunFinalization();


#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif //MAPLE_LIB_CORE_MPL_REFERENCE_H_
