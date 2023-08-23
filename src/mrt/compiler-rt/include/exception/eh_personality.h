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
#ifndef _MPL_EH_PERSONALITY_H
#define _MPL_EH_PERSONALITY_H

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <stdarg.h>
#include <cstdlib>
#include <exception>
#include <typeinfo>

#define EHLOG(level) LOG(level) << "<EH> "

using MrtClass = void*;

namespace maplert {
const unsigned int kAbnormalFrameTag = 0x55555555;
const unsigned int kNoExceptionTag = 0xFFFFFFFF;

typedef enum {
  kNoException = 0,
  kNoFrame     = 1,
  kNormal      = 2,
} EHTableType;
extern "C" {

/* This section defines global identifiers and their values that are associated
 * with interfaces contained in libgcc_s. These definitions are organized into
 * groups that correspond to system headers. This convention is used as a
 * convenience for the reader, and does not imply the existence of these headers,
 * or their content.
 * These definitions are intended to supplement those provided in the referenced
 * underlying specifications.
 * This specification uses ISO/IEC 9899 C Language as the reference programming
 * language, and data definitions are specified in ISO C format. The C language
 * is used here as a convenient notation. Using a C language description of these
 * data objects does not preclude their use by other programming languages.
 * http://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/libgcc-s-ddefs.html
 */
typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8,
} UnwindReasonCode;

typedef enum {
  kSearchPhase = 1,
  kCleanupPhase = 2,
} UnwindAction;

using _Unwind_Word = uintptr_t;
using _Unwind_Ptr = uintptr_t;
using _Unwind_Exception_Class = uintptr_t;
struct _Unwind_Context;
struct _Unwind_Exception;

typedef void (*_Unwind_Exception_Cleanup_Fn)(UnwindReasonCode, struct _Unwind_Exception*);

#if defined(__aarch64__)
struct _Unwind_Exception {
  _Unwind_Exception_Class exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;
  _Unwind_Word private_1;
  _Unwind_Word private_2;
} __attribute__((__aligned__));
#elif defined(__arm__)
struct _Unwind_Control_Block;
#define _Unwind_Exception _Unwind_Control_Block
#endif

typedef UnwindReasonCode (*UnwindTraceFn)(const _Unwind_Context*, void*);

void _Unwind_SetGR(struct _Unwind_Context*, int, _Unwind_Word);

_Unwind_Word _Unwind_GetIP(struct _Unwind_Context*);
void _Unwind_SetIP(struct _Unwind_Context*, _Unwind_Word);
void _Unwind_Resume(struct _Unwind_Exception*);
UnwindReasonCode _Unwind_Backtrace(UnwindTraceFn, void*);

#if defined(__arm__)
typedef enum {
  _UVRSC_CORE  = 0, // INTEGER REGISTER
  _UVRSC_VFP   = 1, // vfp
  _UVRSC_WMMXD = 3, // Intel WMMX data register
  _UVRSC_WMMXC = 4, // Intel WMMX control register
} _Unwind_VRS_RegClass;

typedef enum {
  _UVRSD_UINT32  = 0,
  _UVRSD_VFPX    = 1,
  _UVRSD_UINT64  = 3,
  _UVRSD_FLOAT   = 4,
  _UVRSD_DOUBLE  = 5
} _Unwind_VRS_DataRepresentation;

typedef enum {
  _UVRSR_OK      = 0,
  _UVRSR_NOT_IMPLEMENTED = 1,
  _UVRSR_FAILED  = 2
} _Unwind_VRS_Result;

typedef enum {
  _US_VIRTUAL_UNWIND_FRAME = 0,
  _US_UNWIND_FRAME_STARTING = 1,
  _US_UNWIND_FRAME_RESUME = 2,
  _US_ACTION_MASK = 3,
  _US_FORCE_UNWIND = 8,
  _US_END_OF_STACK = 16
} _Unwind_State;

_Unwind_VRS_Result _Unwind_VRS_Get(_Unwind_Context *context, _Unwind_VRS_RegClass regclass,
    uint32_t regno, _Unwind_VRS_DataRepresentation representation, void *valuep);

_Unwind_VRS_Result _Unwind_VRS_Set(_Unwind_Context *context, _Unwind_VRS_RegClass regclass,
    uint32_t regno, _Unwind_VRS_DataRepresentation representation, void *valuep);

typedef uintptr_t  _Unwind_EHT_Header;
typedef uintptr_t  _uw;

const unsigned int kExceptionClassNameLength = 8;
const unsigned int kBarrierCachePatternSize = 5;
const unsigned int kCleanupCachePatternSize = 4;

// UCB:
struct _Unwind_Control_Block {
  char exception_class[kExceptionClassNameLength];
  void (*exception_cleanup)(UnwindReasonCode, _Unwind_Control_Block*);
  // Unwinder cache, private fields for the unwinder's use
  struct {
    _uw reserved1; // Forced unwind stop fn, 0 if not forced
    _uw reserved2; // Personality routine address
    _uw reserved3; // Saved callsite address
    _uw reserved4; // Forced unwind stop arg
    _uw reserved5;
  }
  unwinder_cache;
  // Propagation barrier cache (valid after phase 1):
  struct {
    _uw sp;
    _uw bitpattern[kBarrierCachePatternSize];
  }
  barrier_cache;
  // Cleanup cache (preserved over cleanup):
  struct {
    _uw bitpattern[kCleanupCachePatternSize];
  }
  cleanup_cache;
  // Pr cache (for pr's benefit):
  struct {
    _uw fnstart;              // function start address
    _Unwind_EHT_Header *ehtp; // pointer to EHT entry header word
    _uw additional;           // additional data
    _uw reserved1;
  }
  pr_cache;
  long long int :0; // Force alignment to 8-byte boundary
};
#endif
}

struct ScanResults {
  uintptr_t      tTypeIndex;            // > 0 catch handler, < 0 exception spec handler, == 0 a cleanup
  uintptr_t      landingPad;            // null -> nothing found, else something found
  bool           caughtByJava;          // set if exception is caught by some java frame
  UnwindReasonCode unwindReason;        // One of _URC_FATAL_PHASE1_ERROR,
                                        //        _URC_FATAL_PHASE2_ERROR,
                                        //        _URC_CONTINUE_UNWIND,
                                        //        _URC_HANDLER_FOUND
  ScanResults() {
    tTypeIndex = 0;
    landingPad = 0;
    caughtByJava = false;
    unwindReason = _URC_FATAL_PHASE1_ERROR;
  }
};

class EHTable {
 public:
  ScanResults results;
  bool CanCatch(const MrtClass catchType, const void *exObj);
  uintptr_t ReadSData4(const uint8_t **p) const;
  uintptr_t ReadULEB128(const uint8_t **data) const;
  uintptr_t ReadTypeData(const uint8_t **data, bool use32Ref = false);
  MrtClass GetMplTypeInfo(uintptr_t tTypeIndex, const uint8_t *classInfo);
  void ScanExceptionTable(UnwindAction actions, bool nativeException, const _Unwind_Exception &unwindException);

  void SetScanResultsValue(const uintptr_t tTypeIndex, const uintptr_t landingPad,
                           const UnwindReasonCode unwindReason, const bool caughtByJava);

  void ParseEHTableHeadInfo(const uint8_t *start);

  EHTable(const uint32_t *funcStart, const uint32_t *funcEnd, const uint32_t *pc) {
    funcEndPoint = funcEnd;
    funcStartPoint = funcStart;
    currentPC = pc;
    if ((*funcEndPoint) == kNoExceptionTag) {
      type = kNoException;
      uintptr_t cleanupOffset = *(funcEndPoint + 1);
      uintptr_t cleanupLabel = 0;
      if (cleanupOffset != 0) {
        cleanupLabel = reinterpret_cast<uintptr_t>(funcStartPoint) + cleanupOffset;
      }
      // Found a cleanup
      SetScanResultsValue(0, cleanupLabel, _URC_HANDLER_FOUND, false);
    } else if ((*funcEndPoint) == kAbnormalFrameTag) {
      type = kNoFrame;
      SetScanResultsValue(0, 0, _URC_HANDLER_FOUND, false);
    } else {
      type = kNormal;
      uintptr_t lsdaOffset = *funcEndPoint;
      ehTableStart = reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(funcStartPoint) + lsdaOffset);
      curPtr = ehTableStart;
      ParseEHTableHeadInfo(curPtr);
    }
  }

 private:
  EHTableType type;
  const uint8_t *curPtr = nullptr;
  const uint8_t *ehTableStart = nullptr;
  const uint32_t *funcStartPoint = nullptr;
  const uint32_t *funcEndPoint = nullptr;
  const uint32_t *currentPC = nullptr;
  const uint8_t *classInfoPoint = nullptr;
  const uint8_t *callSiteTableStart = nullptr;
  const uint8_t *callSiteTableEnd = nullptr;
  const uint8_t *actionTableStart = nullptr;
};

#pragma GCC visibility push(hidden)

#if defined(__aarch64__)
// "MPLJAVA\0" for maple java runtime
static const uint64_t kOurExceptionClass = 0x4d504c4a41564100;
static const uint64_t kGetVendorAndLanguage = 0xFFFFFFFFFFFFFF00;
#elif defined(__arm__)
static const char kOurExceptionClass[kExceptionClassNameLength] = "MPLEC";
static const uint32_t kGetVendorAndLanguage = 0xFFFFFF00;
#endif

#pragma GCC visibility pop

extern "C" {
#if defined(__aarch64__)
__attribute__((__visibility__("default"))) UnwindReasonCode __mpl_personality_v0 (int version,
    UnwindAction actions, uintptr_t exceptionClass, const _Unwind_Exception *unwindException,
    const _Unwind_Context *context);
#elif defined(__arm__)
__attribute__((__visibility__("default"))) UnwindReasonCode __mpl_personality_v0(_Unwind_State state,
    _Unwind_Exception *unwindException, _Unwind_Context *context);
#endif
} // extern "C"
} // namespace maplert
#endif  // _MPL_EH_PERSONALITY_H
