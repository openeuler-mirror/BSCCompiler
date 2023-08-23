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
#ifndef MAPLE_RUNTIME_MM_CONFIG_H
#define MAPLE_RUNTIME_MM_CONFIG_H

#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "securec.h"
#include "mrt_mm_config_common.h"
#include "base/logging.h"

// Enable debug features.  Codes that implement debug features are guarded by
// this macro.  When this macro is set to 0, related codes will become no-ops
// and will have no impact on performance or code size.  Affected features
// include (but are not limited to):
//
// -   Assertion using __MRT_ASSERT (panic.h)
// -   ROS allocator debug features:
//     -   Assertion, verification, dumping
//     -   Bitmap debug in rosallocator
//     -   Part of the statistics
// -   Naive RC collector debug features:
//     -   Inc/Dec from zero detection (?)
//     -   Weak RC checking
//     -   LoadIncRef starvation detection
// -   Saferegion checks
// -   GC phase timer
//
#ifndef __MRT_DEBUG
#define __MRT_DEBUG 1
#endif // __MRT_DEBUG

// Environment variable-affected configuration
//
// Enable EnvConf. This may create a "undisclosed interface" (i.e. security
// exploit) in production, so we provide a way to easily disable it when
// compiling for production.
#ifndef MRT_ALLOW_ENVCONF
#define MRT_ALLOW_ENVCONF 1
#endif

#if MRT_ALLOW_ENVCONF
namespace maplert {
long MrtEnvConf(const char *name, long defaultValue); // do not use directly
}

// Use this macro to get config from environment variable, defaulting to the
// value of a macro.
//
// For example: MRT_ENVCONF(MRT_GCLOG_USE_FILE, MRT_GCLOG_USE_FILE_DEFAULT)
// Will first check if an environment variable "MRT_GCLOG_USE_FILE" is present,
// and is a valid integer, and use its value.  If not present or is not a valid
// integer, it will fall back to the current value of the MRT_GCLOG_USE_FILE
// macro.  This lets the user override configuration at run time, most useful
// for debugging.
#define MRT_ENVCONF(conf, defaultValue) maplert::MrtEnvConf(#conf, defaultValue)
#else
#define MRT_ENVCONF(conf) (conf)
#endif

// configuration control for Maple runtime memory manager
// HEAP_OBJ_CHECK, used for debugging.
#ifndef HEAP_OBJ_CHECK // When set, INC, DEC and other RC operations will print
#define HEAP_OBJ_CHECK 0 // detailed error messages when encountering a non-heap obj.
#endif

#ifndef HEAP_PROFILE
#define HEAP_PROFILE 0
#endif // HEAP_PROFILE

#ifndef SAFEREGION_CHECK // When set, RC will print backtrace and abort the program
#define SAFEREGION_CHECK __MRT_DEBUG_COND_FALSE // if mutator execute inc, dec or other RC operations in saferegion.
#endif

#ifndef RC_PROFILE
#define RC_PROFILE 0
#endif // RC_PROFILE

#ifndef ENABLE_HPROF
#define ENABLE_HPROF 1
#endif

#ifndef LOG_ALLOC_TIMESTAT // Set to 1 to log time stats for obj allocation
#define LOG_ALLOC_TIMESTAT 0 // and release in allocator
#endif

// Set to 1 to enable collecting hot object data for RC.
#ifndef RC_HOT_OBJECT_DATA_COLLECT
#define RC_HOT_OBJECT_DATA_COLLECT 0
#endif // RC_HOT_OBJECT_DATA_COLLECT

#ifndef BT_CLEANUP_PROFILE // Back-up tracing cleanup profiling
#define BT_CLEANUP_PROFILE 1
#endif // BT_CLEANUP_PROFILE

#ifndef RC_TRACE_OBJECT
#define RC_TRACE_OBJECT __MRT_DEBUG_COND_FALSE
#endif // RC_TRACE_OBJECT

#ifndef PATTERN_FROM_BACKUP_TRACING_DEFAULT
#define PATTERN_FROM_BACKUP_TRACING_DEFAULT 0
#endif

// Enforcing MRT_GCFini{Global,ThreadLocal}
#ifndef MRT_ENFORCE_FINI // When 1, not calling MRT_GCFini{Global,ThreadLocal} will abort and print error
#define MRT_ENFORCE_FINI 0 // When 0, We will call MRT_GCFini{Global,ThreadLocal} for the higher level
#endif // MRT_ENFORCE_FINI

// Selection of different allocator
// define MRT_ALLOCATOR_MALLOC 1
#define MRT_ALLOCATOR_ROS 3

#ifndef MRT_ALLOCATOR
#define MRT_ALLOCATOR MRT_ALLOCATOR_ROS
#endif

// Enable/disable systrace in the allocator slow path
#ifndef MRT_SYSTRACE_ALLOCATOR
#define MRT_SYSTRACE_ALLOCATOR 0
#endif

// configuration for use-after-free debugging
//
// When enabled, newobj, freeobj, inc and dec will check if the object address
// to work with actually points to a live object.
#ifndef MRT_REF_VALIDITY_CHECK
#define MRT_REF_VALIDITY_CHECK 1 // Do checking by default. Can be disabled.
#endif // MRT_REF_VALIDITY_CHECK

// This enables assertions at obj free
#ifndef MRT_DEBUG_DOUBLE_FREE
#define MRT_DEBUG_DOUBLE_FREE 1
#endif

// GCLog: logs specific to GC
//
// If this is on, write GC log to a disk file.  This is useful when debugging,
// because logcat may be flooded by messages from other processes.
//
// If off, GCLog will not do anything.
//
// Can be overridden by the environment variable of the same name.
#ifndef MRT_GCLOG_USE_FILE_DEFAULT
#define MRT_GCLOG_USE_FILE_DEFAULT 0
#endif // MRT_GCLOG_USE_FILE_DEFAULT

// If true, the process will open the GC log file and write to it on startup.
// Otherwise, it will only open GC log file on fork(), i.e. when Zygote creates
// a sub-process.
//
// Can be overridden by the environment variable of the same name.
#ifndef MRT_GCLOG_OPEN_ON_STARTUP_DEFAULT
#define MRT_GCLOG_OPEN_ON_STARTUP_DEFAULT 0
#endif // MRT_GCLOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_RCTRACELOG_USE_FILE_DEFAULT
#define MRT_RCTRACELOG_USE_FILE_DEFAULT RC_TRACE_OBJECT
#endif // MRT_RCTRACELOG_USE_FILE_DEFAULT

#ifndef MRT_RCTRACELOG_OPEN_ON_STARTUP_DEFAULT
#define MRT_RCTRACELOG_OPEN_ON_STARTUP_DEFAULT RC_TRACE_OBJECT
#endif // MRT_RCTRACELOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_RPLOG_USE_FILE_DEFAULT
#define MRT_RPLOG_USE_FILE_DEFAULT 0
#endif // MRT_RPLOG_USE_FILE_DEFAULT

#ifndef MRT_RPLOG_OPEN_ON_STARTUP_DEFAULT
#define MRT_RPLOG_OPEN_ON_STARTUP_DEFAULT 0
#endif // MRT_RPLOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_CYCLELOG_USE_FILE_DEFAULT
#define MRT_CYCLELOG_USE_FILE_DEFAULT 0
#endif // MRT_CYCLELOG_USE_FILE_DEFAULT

#ifndef MRT_CYCLELOG_OPEN_ON_STARTUP_DEFAULT
#define MRT_CYCLELOG_OPEN_ON_STARTUP_DEFAULT 0
#endif // MRT_CYCLELOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_ALLOCFRAGLOG_USE_FILE_DEFAULT
#define MRT_ALLOCFRAGLOG_USE_FILE_DEFAULT 0
#endif // MRT_ALLOCFRAGLOG_USE_FILE_DEFAULT

#ifndef MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP_DEFAULT
#define MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP_DEFAULT 0
#endif // MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_ALLOCATORLOG_USE_FILE_DEFAULT
#define MRT_ALLOCATORLOG_USE_FILE_DEFAULT 0
#endif // MRT_ALLOCFRAGLOG_USE_FILE_DEFAULT

#ifndef MRT_ALLOCATOR_OPEN_ON_STARTUP_DEFAULT
#define MRT_ALLOCATOR_OPEN_ON_STARTUP_DEFAULT 0
#endif // MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_MIXLOG_USE_FILE_DEFAULT
#define MRT_MIXLOG_USE_FILE_DEFAULT 0
#endif // MRT_ALLOCFRAGLOG_USE_FILE_DEFAULT

#ifndef MRT_MIXLOG_OPEN_ON_STARTUP_DEFAULT
#define MRT_MIXLOG_OPEN_ON_STARTUP_DEFAULT 0
#endif // MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP_DEFAULT

#ifndef MRT_STDERRLOG_USE_FILE
#define MRT_STDERRLOG_USE_FILE 0
#endif // MRT_ALLOCFRAGLOG_USE_FILE

#ifndef MRT_STDERR_OPEN_ON_STARTUP
#define MRT_STDERR_OPEN_ON_STARTUP 0
#endif // MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP

// if set to 1 by the environment variable, MarkSweepCollector::concurrentMark and concurrentSweep will be false
#ifndef MRT_IS_NON_CONCURRENT_GC_DEFAULT
#define MRT_IS_NON_CONCURRENT_GC_DEFAULT 0
#endif // MRT_IS_NON_CONCURRENT_GC_DEFAULT

#define LOAD_INC_RC_MASK 1 // can only use lower 3bits on 64bit-platform

#ifdef USE_32BIT_REF
#define GRT_DEADVALUE (0xdeaddeadul)
#else
#define GRT_DEADVALUE (0xdeaddeaddeaddeadul)
#endif // USE_32BIT_REF

// ---------------------------------------------------
// NOTE: Don't forget update 'duplicateFunc.s'
// if you changed HEAP_START or HEAP_END.
// ---------------------------------------------------

#ifdef __aarch64__
// The first 64KB is protected by SELinux.
#define HEAP_START (1u << 16)
#define HEAP_END (1ul << 31) // max of 2GB space
#define PERM_BEGIN (9ul << 28) // perm start from 2.25G
#define PERM_END (3ul << 30) // perm end at 3G
#elif defined(__arm__)
// The first 64KB is protected by SELinux.
// 0~256MB is reserved to encode offset in mfile.
#define PERM_BEGIN (256ul << 20)
#define PERM_END (1ul << 30)
#define HEAP_START PERM_END
#define HEAP_END (2ul << 30)
#endif // __aarch64__

#ifndef PERMISSIVE_HEAP_COND
#define PERMISSIVE_HEAP_COND 0 // disabled for portability reason
#endif
namespace maplert {
constexpr uint64_t kMrtHeapAlign = 8;
// Maple uses i32 for array length and can be negative
constexpr uint32_t kMrtMaxArrayLength = ((1UL << 31) - 1); // the max of int32 is 2 ^ 31 - 1
constexpr uint32_t kMfileHighAddress = 0xfe000000;
}

#ifdef USE_32BIT_REF
#define ALLOC_USE_FAST_PATH (!CONFIG_JSAN && \
                             !RC_HOT_OBJECT_DATA_COLLECT && \
                             !RC_TRACE_OBJECT && \
                             !LOG_ALLOC_TIMESTAT)
#else
#define ALLOC_USE_FAST_PATH false
#endif

#ifndef LIKELY
#define LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#endif // MAPLE_RUNTIME_MMCONFIG_H
