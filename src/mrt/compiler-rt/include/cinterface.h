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
#ifndef MAPLE_RUNTIME_CINTERFACE_H
#define MAPLE_RUNTIME_CINTERFACE_H

#include <iostream>
#include <cstdlib>
#include <functional>
#include <utility>
#include <vector>
#include <map>
#include <string>
#include "gc_roots.h"
#include "address.h"
#include "mrt_reference_api.h"
#include "heap_stats.h"

// These functions form the interface for the .s code generated from mplcg.
// A criteria for inclusion is that if the function is callable from generated .s,
// and is front-end independent (i.e. not specific to dex2mpl), it should be here.
// chelper.h shall help C programmers, including the developers
#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

// Most functions are simple wrappers of methods of global or thread-local
// objects defined in chosen.h/cpp.  Read cinterface.cpp for more information.
//
// Gets the polling page address.
void *MRT_GetPollingPage();

// returns the approximate total number of live objects
size_t MRT_TotalHeapObj();

// executes heap trim
void MRT_Trim(bool aggressive);

// type of permanent space allocation
enum MetaTag : uint16_t {
  kClassMetaData = 0,
  kFieldMetaData,
  kMethodMetaData,
  kITabMetaData,
  kNativeStringData,
  kMetaTagNum
};

enum DecoupleTag : uint16_t {
  kITab = 0,
  kITabAggregate,
  kVTab,
  kVTabArray,
  kTagMax
};
static const std::string kDecoupleTagNames[kTagMax] = {
    "itab", "itab aggregate",
    "vtab", "vtab array",
};
address_t MRT_AllocFromPerm(size_t size);
address_t MRT_AllocFromMeta(size_t size, MetaTag metaTag);
address_t MRT_AllocFromDecouple(size_t size, DecoupleTag tag);

void MRT_FreeObj(address_t obj);
void MRT_ResetHeapStats();

bool MRT_IsPermJavaObj(address_t obj);

// GC related interface
void MRT_PrintRCStats();
void MRT_ResetRCStats();

size_t MRT_GetNativeAllocBytes();
void MRT_SetNativeAllocBytes(size_t size);

void MRT_ClassInstanceNum(std::map<std::string, long> &objNameCntMp);

// collector specific JNI interface
// Unsafe
bool MRT_UnsafeCompareAndSwapObject(address_t obj, ssize_t offset,
                                    address_t expectedValue, address_t newValue);
address_t MRT_UnsafeGetObjectVolatile(address_t obj, ssize_t offset);
address_t MRT_UnsafeGetObject(address_t obj, ssize_t offset);
void MRT_UnsafePutObject(address_t obj, ssize_t offset, address_t newValue);
void MRT_UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue);
void MRT_UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue);

// barriers for runtime code.
address_t MRT_LoadRefField(address_t obj, address_t *fieldAddr);
address_t MRT_LoadVolatileField(address_t obj, address_t *fieldAddr);
void MRT_WriteRefField(address_t obj, address_t *field, address_t value);
void MRT_WriteVolatileField(address_t obj, address_t *objAddr, address_t value);

address_t MRT_LoadRefFieldCommon(address_t obj, address_t *fieldAddr); // only used in naiverc

// write barrier
void MRT_WriteRefFieldNoRC(address_t obj, address_t *field, address_t value);
void MRT_WriteRefFieldNoDec(address_t obj, address_t *field, address_t value);
void MRT_WriteRefFieldNoInc(address_t obj, address_t *field, address_t value);

void MRT_WriteVolatileFieldNoInc(address_t obj, address_t *objAddr, address_t value);
void MRT_WriteVolatileFieldNoDec(address_t obj, address_t *objAddr, address_t value);
void MRT_WriteVolatileFieldNoRC(address_t obj, address_t *objAddr, address_t value);

// RC weak field processing
// static not supported
void MRT_WriteWeakField(address_t obj, address_t *field, address_t value, bool isVolatile);
address_t MRT_LoadWeakField(address_t obj, address_t *field, bool isVolatile);
address_t MRT_LoadWeakFieldCommon(address_t obj, address_t *field);

// For referent processing
void MRT_WriteReferentField(address_t obj, address_t *fieldAddr, address_t value, bool isResurrectWeak);
// Load Referent can only happen in soft/weak/weak global, weak_global has special handling
// Used in java mutator thread
address_t MRT_LoadReferentField(address_t obj, address_t *fieldAddr);

// write barrier for local reference variable update.
void MRT_WriteRefVar(address_t *var, address_t value); // not used now, undefined
void MRT_WriteRefVarNoInc(address_t *var, address_t value); // not used, undefined

// Call this function when we are going to renew an dead object,
// for example: reuse string from pool in NewStringUtfFromPool().
// when concurrent mark is running, this function will mark the object to
// prevent it from being swept by GC.
void MRT_PreRenewObject(address_t obj);

void MRT_SetTracingObject(address_t obj);

// release local reference variable, only naive rc need this.
void MRT_ReleaseRefVar(address_t obj);

bool MRT_IsValidObjectAddress(address_t obj);

// Trigger GC. Callable from mutator threads.  This function may or may not
// block, depending on the specific reason.  If it blocks, it will return when
// it has finished GC.
//
// reason: The specific reason for performing GC.
void MRT_TriggerGC(maplert::GCReason reason);

bool MRT_IsNaiveRCCollector();

// Returns true if current thread is GC thread.
bool MRT_IsGcThread();

bool MRT_IsGcRunning();

bool MRT_FastIsValidObjAddr(address_t obj);

#if LOG_ALLOC_TIMESTAT
void MRT_PrintAllocTimers();
void MRT_ResetAllocTimers();
#endif
void MRT_DebugShowCurrentMutators();
size_t MRT_AllocSize();
size_t MRT_AllocCount();
size_t MRT_FreeSize();
size_t MRT_FreeCount();

void MRT_DumpHeap(const std::string &tag);

// dump RC and GC information into stream os
void MRT_DumpRCAndGCPerformanceInfo(std::ostream &os);

void MRT_DumpRCAndGCPerformanceInfo_Stderr();

void MRT_VisitAllocatedObjects(maple::rootObjectFunc f);

// Cycle pattern interface
void MRT_DumpDynamicCyclePatterns(std::ostream &os, size_t limit);
// Send a job that will be executed by the reference processor thread when it
// wakes up. Intended to perform cycle pattern saving/loading/writing signature etc.
// Implemented in reference-processor.cpp
void MRT_SendCyclePatternJob(std::function<void()> job);
void MRT_SetPeriodicSaveCpJob(std::function<void()> job);
void MRT_SetPeriodicLearnCpJob(std::function<void()> job);
void MRT_SendSaveCpJob();
bool MRT_IsCyclePatternUpdated();

// Send a background gc job to reference processor when process state change to jank imperceptible.
void MRT_SendBackgroundGcJob(bool force);


void MRT_UpdateProcessState(ProcessState processState, bool isSystemServer);

// Set a callback function which is called after GC finished, but before
// starting the world.  Useful for performing cleaning up after a GC.
void MRT_DumpStaticField(std::ostream &os);
void MRT_WaitGCStopped();
#if RC_TRACE_OBJECT
void TraceRefRC(address_t obj, uint32_t rc, const char *msg);
#endif
void MRT_SetAllocRecordingCallback(std::function<void(address_t, size_t)> callback);

// traverse 'objects' allocated in the perm space
void MRT_VisitDecoupleObjects(maple::rootObjectFunc f);

#ifdef __cplusplus
} // namespace maplert
} // extern "C"
#endif // __cplusplus

#endif // MAPLE_RUNTIME_CINTERFACE_H
