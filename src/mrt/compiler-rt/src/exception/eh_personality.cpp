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
#include "exception/eh_personality.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>
#include <dlfcn.h>
#include "exception/mrt_exception.h"
#include "exception/mpl_exception.h"
#include "linker_api.h"

namespace maplert {
const uint8_t TTYPE_ENCODING = 0x9B;
const uint8_t LP_START_ENCODING = 0xFF;
const uint8_t CALL_SITE_ENCODING = 0x01;

bool EHTable::CanCatch(const MrtClass catchType, const void *exObj) {
  if (exObj == nullptr) {
    EHLOG(FATAL) << "exception object is null" << maple::endl;
  }
  return reinterpret_cast<const MObject*>(exObj)->IsInstanceOf(*reinterpret_cast<MClass*>(catchType));
}

uintptr_t EHTable::ReadSData4(const uint8_t **p) const {
  uint32_t value;
  size_t size = sizeof(uint32_t);
  errno_t result = memcpy_s(&value, size, *p, size);
  if (result != EOK) {
    EHLOG(ERROR) << "memcpy_s() in ReadSData4() return " << result << "rather than 0." << maple::endl;
    return reinterpret_cast<uintptr_t>(nullptr);
  }
  *p += size;
  return static_cast<uintptr_t>(value);
}

uintptr_t EHTable::ReadULEB128(const uint8_t **data) const {
  uintptr_t result = 0;
  uintptr_t shift = 0;
  const uintptr_t shiftLength = 7;
  unsigned char byte;
  const uint8_t *p = *data;
  do {
    byte = *p++;
    result |= static_cast<uintptr_t>(byte & 0x7F) << shift;
    shift += shiftLength;
  } while (byte & 0x80);
  *data = p;
  return result;
}

uintptr_t EHTable::ReadTypeData(const uint8_t **data, bool use32Ref) {
  const uint8_t *typePoint = *data;
  // first get value
  uintptr_t  result = ReadSData4(&typePoint);
  // then add relative offset
  if (result != 0) {
    result += reinterpret_cast<uintptr_t>(*data);
  }
  if (result != 0) {
    if (use32Ref) {
#ifdef LINKER_LAZY_BINDING
      (void)MRT_RequestLazyBindingForInitiative(reinterpret_cast<void*>(result));
#endif // LINKER_LAZY_BINDING
      result = static_cast<uintptr_t>(*(reinterpret_cast<uint32_t*>(result)));
    } else {
#ifdef LINKER_LAZY_BINDING
      (void)MRT_RequestLazyBindingForInitiative(reinterpret_cast<void*>(result));
#endif // LINKER_LAZY_BINDING
      result = *(reinterpret_cast<uintptr_t*>(result));
    }
  }
  *data = typePoint;
  return result;
}

MrtClass EHTable::GetMplTypeInfo(uintptr_t tTypeIndex, const uint8_t *classInfo) {
  if (classInfo == nullptr) {
    EHLOG(FATAL) << "classInfo is 0 in GetMplTypeInfo" << maple::endl;;
  }
#if defined(__aarch64__)
  if (UINT64_MAX / sizeof(int) < tTypeIndex) {
#elif defined(__arm__)
  if (UINT32_MAX / sizeof(int) < tTypeIndex) {
#endif
    EHLOG(FATAL) << "tTypeIndex * sizeof(int) > UINT64_MAX" << maple::endl;
  } else {
    tTypeIndex *= sizeof(int);
  }
  classInfo -= tTypeIndex;
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
  return reinterpret_cast<MrtClass>(ReadTypeData(&classInfo, true));
#else
  return reinterpret_cast<MrtClass>(ReadTypeData(&classInfo));
#endif // LINKER_32BIT_REF_FOR_DEF_UNDEF
}

extern "C" {
ATTR_NO_SANITIZE_ADDRESS
static void SetRegisters(const _Unwind_Exception &unwindException, const _Unwind_Context &context,
                         const ScanResults &results) {
  MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(unwindException);

#if defined(__arm__)
  const _Unwind_Exception *unwindExceptionAddress = &unwindException;
  _Unwind_VRS_Set(const_cast<_Unwind_Context*>(&context), _UVRSC_CORE, kR0, _UVRSD_UINT32,
                  reinterpret_cast<uintptr_t*>(&unwindExceptionAddress));
  _Unwind_VRS_Set(const_cast<_Unwind_Context*>(&context), _UVRSC_CORE, kR1, _UVRSD_UINT32,
                  &exceptionHeader->tTypeIndex);
  _Unwind_VRS_Set(const_cast<_Unwind_Context*>(&context), _UVRSC_CORE, kPC, _UVRSD_UINT32,
                  const_cast<uintptr_t*>(&results.landingPad));
#else
  _Unwind_SetGR(const_cast<_Unwind_Context*>(&context), __builtin_eh_return_data_regno(0),
                reinterpret_cast<uintptr_t>(&unwindException));
  _Unwind_SetGR(const_cast<_Unwind_Context*>(&context), __builtin_eh_return_data_regno(1),
                static_cast<uintptr_t>(exceptionHeader->tTypeIndex));
  _Unwind_SetIP(const_cast<_Unwind_Context*>(&context), results.landingPad);
#endif
}
} // extern "C"

// maple exception table:
//
// No exception function >>
//      .word    0xFFFFFFFF
//      .word    .Label.Cleanup
//
// No frame function(leaf function) >>
//      .word    0x55555555
//
// exception function >>
//      .word    mplETableOffset
//      .byte    lpStartEncoding
//      .byte    ttypeEncoding
//      .uleb128 classInfoOffset
//      .byte    callSiteEncoding
//      .uleb128 callSiteTableLength
//     Call Site Table
//      .uleb    start
//      .uleb    length
//      .uleb    ladingPad
//      .uleb    actionEntry
//      ....
//     Action Table
//      .byte    tTypeIndex
//      .byte    ttypeOffset
//      ....
//     Exception Class Info
//      .4byte   class info
//

void EHTable::ParseEHTableHeadInfo(const uint8_t *mLSDA) {
  if (mLSDA == nullptr) {
    LOG(FATAL) << "mLSDA is nullptr in function ScanExceptionTable for method" << maple::endl;
  }
  uint8_t lpStartEncoding = *mLSDA++;
  MplCheck(lpStartEncoding == LP_START_ENCODING);

  uint8_t ttypeEncoding = *mLSDA++;
  MplCheck(ttypeEncoding == TTYPE_ENCODING);
  uintptr_t classInfoOffset = ReadULEB128(&mLSDA);
  classInfoPoint = mLSDA + classInfoOffset;

  // Walk call-site table looking for range that includes current PC.
  uint8_t callSiteEncoding = *mLSDA++;
  MplCheck(callSiteEncoding == CALL_SITE_ENCODING);
  uint32_t callSiteTableLength = static_cast<uint32_t>(ReadULEB128(&mLSDA));
  callSiteTableStart = mLSDA;
  callSiteTableEnd = callSiteTableStart + callSiteTableLength;

  actionTableStart = callSiteTableEnd;
  curPtr = mLSDA;
}

void EHTable::ScanExceptionTable(UnwindAction actions, bool nativeException,
                                 const _Unwind_Exception &unwindException) {
  MplCheck(nativeException);
  // actions must be either _UA_SEARCH_PHASE or _UA_SEARCH_PHASE.
  // either action shares the same scanning strategy.
  MplCheck((static_cast<unsigned int>(actions) & static_cast<int>(kSearchPhase)) ||
           (static_cast<unsigned int>(actions) & static_cast<int>(kCleanupPhase)));

  if (type == kNoException || type == kNoFrame) {
    return;
  }

  uintptr_t ip = reinterpret_cast<uintptr_t>(currentPC);
  // we should always adjust ip by -1 to make it point to the instruction which
  // raises the signal or performs a function call, because this is the precise
  // site triggering an exception.
  // Note in PrepareToHandleJavaSignal we have already bias the pc to the next
  // instruction of the instruction which raises a signal, like what bl instruction
  // does. so it is safe to do so.
  ip -= kPrevInsnOffset;

  // Get beginning current frame's code (as defined by the emitted dwarf code)
  uintptr_t funcStart = reinterpret_cast<uintptr_t>(funcStartPoint);
  uintptr_t ipOffset = ip - funcStart;

  jthrowable thrownObject = GetThrownObject(unwindException);
  MplCheck(thrownObject != nullptr);

  while (curPtr < callSiteTableEnd) {
    // There is one entry per call site.
    // The call sites are non-overlapping in [start, start+length)
    // The call sites are ordered in increasing value of start
    uintptr_t start = ReadULEB128(&curPtr);
    uintptr_t length = ReadULEB128(&curPtr);
    uintptr_t landingPad = ReadULEB128(&curPtr);
    uintptr_t actionEntry = ReadULEB128(&curPtr);
    if ((start <= ipOffset) && (ipOffset < (start + length))) {
      // Found the call site containing ip.
      if (landingPad == 0) {
        // No handler here
        results.unwindReason = _URC_CONTINUE_UNWIND;
        return;
      }
      landingPad = reinterpret_cast<uintptr_t>(funcStartPoint) + landingPad;
      if (actionEntry == 0) {
        // Found a cleanup
        SetScanResultsValue(0, landingPad, _URC_HANDLER_FOUND, false);
        return;
      }
      // Convert 1-based byte offset into
      const uint8_t *action = actionTableStart + (actionEntry - 1);
      // Scan action entries until you find a matching handler, or the end of action list.
      // action list is a sequence of pairs (type index, offset of next action).
      // if offset of next action is 0, the end of current action list is reached.
      // right now type index would not be 0 for maple exception handling.
      for (;;) {
        uintptr_t tTypeIndex = *action++;
        MplCheck(tTypeIndex != 0);
        if (tTypeIndex > 0) {
          // this is a catch (T) clause
          const MrtClass catchType = GetMplTypeInfo(static_cast<uintptr_t>(tTypeIndex),
                                                    classInfoPoint);
          MplCheck(catchType != nullptr);
          if (CanCatch(catchType, thrownObject)) {
            // Found a matching handler
            // Save state and return _URC_HANDLER_FOUND
            SetScanResultsValue(tTypeIndex, landingPad, _URC_HANDLER_FOUND, true);
            return;
          }
        }

        const uint8_t actionOffset = *action;
        // reach the end of action list
        if (actionOffset == 0) {
          // End of action list, no matching handler or cleanup found,
          // which means the thrown exception can not be caught by this catch clause.
          // For C++, we will return _URC_CONTINUE_UNWIND.
          // For maple, we will continue to search call site table until we reach
          // the last call site entry which is the cleanup entry.
          break;
        } else if (actionOffset == 125) {
          // According to the sleb encoding format, when actionOffset is equal to 125,
          // it is equivalent to action -3, and the parsing process of sleb reading is omitted here.
          action -= 3;
        }
      } // there is no break out of this loop, only return end of while
    }
  } // there might be some tricky cases which break out of this loop

  // It is possible that no eh table entry specify how to handle
  // this exception. By spec, terminate it immediately.
  EHLOG(FATAL) << "ScanExceptionTable failed" << maple::endl;
}

// set ScanResults Value
void EHTable::SetScanResultsValue(const uintptr_t tTypeIndex, const uintptr_t landingPad,
                                  const UnwindReasonCode unwindReason, const bool caughtByJava) {
  results.tTypeIndex = tTypeIndex;
  results.landingPad = landingPad;
  results.unwindReason = unwindReason;
  results.caughtByJava = caughtByJava;
}

extern "C" {
static void DumpInfoPersonality(int version, const _Unwind_Exception &unwindException, int line) {
  EHLOG(ERROR) << "personality error line [" << line << "]" << maple::endl;
  EHLOG(ERROR) << "version: " << version << maple::endl;
  MRT_DumpException(GetThrownObject(unwindException), nullptr);
  MplDumpStack("\n DumpInfoPersonality");
}

#if defined(__aarch64__)
static void DumpFrameInfo(UnwindAction actions, const _Unwind_Context &context) {
  Dl_info addrInfo;
  addrInfo.dli_sname = nullptr;
  std::string unwindAction;
  if (static_cast<unsigned int>(actions) & static_cast<int>(kSearchPhase)) {
    unwindAction = "unwind actions: kSearchPhase  ";
  }
  if (static_cast<unsigned int>(actions) & static_cast<int>(kCleanupPhase)) {
    unwindAction = "unwind actions: kCleanupPhase  ";
  }
  uintptr_t pc;
  pc = _Unwind_GetIP(const_cast<_Unwind_Context*>(&context));
  int dladdrOk = dladdr(reinterpret_cast<void*>(pc), &addrInfo);
  if (dladdrOk && addrInfo.dli_sname) {
    EHLOG(INFO) << unwindAction << "function name of current frame: " << addrInfo.dli_sname << maple::endl;
  } else {
    EHLOG(INFO) << unwindAction << "pc of current frame: 0x" << std::hex << pc << std::dec << maple::endl;
  }
}

UnwindReasonCode __n2j_stub_personality(int version, UnwindAction actions, uintptr_t exceptionClass,
                                        const _Unwind_Exception *unwindException, const _Unwind_Context *context) {
  if (unwindException == nullptr) {
    EHLOG(FATAL) << "unwindException is nullptr!" << maple::endl;
  }
  if (version != 1 || context == nullptr) {
    DumpInfoPersonality(version, *unwindException, __LINE__);
    return _URC_FATAL_PHASE1_ERROR;
  }
  bool nativeException = (exceptionClass & kGetVendorAndLanguage) == (kOurExceptionClass & kGetVendorAndLanguage);

  if (nativeException && (static_cast<unsigned int>(actions) & static_cast<unsigned int>(kCleanupPhase))) {
    // that n2j stub is the last chance to handle java exception.
    // so we pretend that n2j frame catches this exception.
    MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(*unwindException);
    exceptionHeader->tTypeIndex = 0;
    ScanResults results;
    uintptr_t ip;
    ip = _Unwind_GetIP(const_cast<_Unwind_Context*>(context));
    if ((ip & 1) == 1) {
      ip--;
    }
    results.landingPad = reinterpret_cast<uintptr_t>(ip);

    // when the stack backtracks to native frame, print the java stack and all the stack
    // the five line can be deleted for performance
    if (VLOG_IS_ON(eh)) {
      MRT_DumpException(GetThrownObject(*unwindException));
      MplDumpStack("call stack for non-caught exception:");
    }

    MRT_ThrowExceptionUnsafe(GetThrownObject(*unwindException));
    MRT_ClearThrowingException();

    // set the continuation with the handler
    SetRegisters(*unwindException, *context, results);
    free(GetThrownException(*unwindException));
    return _URC_INSTALL_CONTEXT;
  }

  // should not reach here
  DumpInfoPersonality(version, *unwindException, __LINE__);
  return _URC_FATAL_PHASE1_ERROR;
}
#elif defined(__arm__)
UnwindReasonCode __n2j_stub_personality(_Unwind_State state, _Unwind_Exception *unwindException,
                                        _Unwind_Context *context) {
  if (unwindException == nullptr) {
    EHLOG(FATAL) << "unwindException is nullptr!" << maple::endl;
  }
  if (context == nullptr) {
    DumpInfoPersonality(state, *unwindException, __LINE__);
    return _URC_FATAL_PHASE1_ERROR;
  }

  if (state == (_US_VIRTUAL_UNWIND_FRAME | _US_FORCE_UNWIND)) {
    constexpr int contextFPOffset = 12;
    constexpr int contextPCOffset = 16;
    uintptr_t *callerFP = *(reinterpret_cast<uintptr_t**>(context) + contextFPOffset);
    uintptr_t *currentPC = reinterpret_cast<uintptr_t*>(context) + contextPCOffset;
    *currentPC = *(callerFP + 1);
    uintptr_t *currentFP = reinterpret_cast<uintptr_t*>(context) + contextFPOffset;
    *currentFP = *callerFP;
    return _URC_CONTINUE_UNWIND;
  }
  // that n2j stub is the last chance to handle java exception.
  // so we pretend that n2j frame catches this exception.
  MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(*unwindException);
  exceptionHeader->tTypeIndex = 0;
  ScanResults results;
  uintptr_t ip;
  _Unwind_VRS_Get(const_cast<_Unwind_Context*>(context), _UVRSC_CORE, kPC, _UVRSD_UINT32, &ip);
  if ((ip & 1) == 1) {
    ip--;
  }
  results.landingPad = reinterpret_cast<uintptr_t>(ip);

  // when the stack backtracks to native frame, print the java stack and all the stack
  // the five line can be deleted for performance
  if (VLOG_IS_ON(eh)) {
    MRT_DumpException(GetThrownObject(*unwindException));
    MplDumpStack("call stack for non-caught exception:");
  }

  MRT_ThrowExceptionUnsafe(GetThrownObject(*unwindException));
  MRT_ClearThrowingException();

  // set the continuation with the handler
  SetRegisters(*unwindException, *context, results);
  free(GetThrownException(*unwindException));
  return _URC_INSTALL_CONTEXT;
}
#endif

#if defined(__aarch64__)
// this personality is invoked to enter the general handler of the java frame on stack top.
UnwindReasonCode __mpl_personality_v0(int version, UnwindAction actions, uintptr_t exceptionClass,
                                      const _Unwind_Exception *unwindException, const _Unwind_Context *context) {
  if (unwindException == nullptr) {
    EHLOG(FATAL) << "unwindException is nullptr!" << maple::endl;
  }
  if (version != 1 || context == nullptr) {
    DumpInfoPersonality(version, *unwindException, __LINE__);
    return _URC_FATAL_PHASE1_ERROR;
  }

  bool nativeException = (exceptionClass & kGetVendorAndLanguage) == (kOurExceptionClass & kGetVendorAndLanguage);

  MplCheck(nativeException);
  MplCheck((static_cast<unsigned int>(actions) & static_cast<int>(kCleanupPhase)) != 0);

  const uint32_t *pc = reinterpret_cast<const uint32_t*>(_Unwind_GetIP(const_cast<_Unwind_Context*>(context)));
  const uint32_t *mLSDA = nullptr;
  const uint32_t *startPC = nullptr;
  LinkerLocInfo info;
  if (LinkerAPI::Instance().LocateAddress(pc, info, false)) {
    startPC = reinterpret_cast<const uint32_t*>(info.addr);
    mLSDA = reinterpret_cast<const uint32_t*>((uintptr_t)info.addr + info.size);
  } else {
    EHLOG(INFO) << "pc must be java method written with assembly" << maple::endl;
    return _URC_CONTINUE_UNWIND;
  }
  EHTable ehTable(startPC, mLSDA, pc);
  ehTable.ScanExceptionTable(actions, nativeException, *unwindException);
  // If the stack-top java frame is abnormal, eh still calls __mpl_personality_v0 now because .cfi_personality.
  if (ehTable.results.unwindReason == _URC_HANDLER_FOUND && ehTable.results.landingPad) {
    MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(*unwindException);
    if (VLOG_IS_ON(eh)) {
      DumpFrameInfo(actions, *context);
      EHLOG(INFO) << "Catch Exception Succeeded : " <<
          reinterpret_cast<MObject*>(GetThrownObject(*unwindException))->GetClass()->GetName() << maple::endl;
    }
    SetRegisters(*unwindException, *context, ehTable.results);
    if (!exceptionHeader->caughtByJava) {
      free(GetThrownException(*unwindException));
    } // otherwise exception wrapper is freed in begin_catch
    return _URC_INSTALL_CONTEXT;
  } else {
    // control flow reaches here if top-stack java frame is abnormal
    return _URC_CONTINUE_UNWIND;
  }
}

#elif defined(__arm__)
UnwindReasonCode __mpl_personality_v0(_Unwind_State state, _Unwind_Exception *unwindException,
                                      _Unwind_Context *context) {
  if (unwindException == nullptr || context == nullptr) {
    return _URC_FATAL_PHASE1_ERROR;
  }

  if (state == (_US_VIRTUAL_UNWIND_FRAME | _US_FORCE_UNWIND)) {
    constexpr int contextFPOffset = 12;
    constexpr int contextPCOffset = 16;
    uintptr_t *callerFP = *(reinterpret_cast<uintptr_t**>(context) + contextFPOffset);
    uintptr_t *currentPC = reinterpret_cast<uintptr_t*>(context) + contextPCOffset;
    *currentPC = *(callerFP + 1);
    uintptr_t *currentFP = reinterpret_cast<uintptr_t*>(context) + contextFPOffset;
    *currentFP = *callerFP;
    return _URC_CONTINUE_UNWIND;
  }
  const uint32_t *pc;
  _Unwind_VRS_Get(const_cast<_Unwind_Context*>(context), _UVRSC_CORE, kPC, _UVRSD_UINT32, &pc);
  const uint32_t *mLSDA = nullptr;
  const uint32_t *startPC = nullptr;
  LinkerLocInfo info;
  if (LinkerAPI::Instance().LocateAddress(pc, info, false)) {
    startPC = reinterpret_cast<const uint32_t*>(info.addr);
    mLSDA = reinterpret_cast<const uint32_t*>((uintptr_t)info.addr + info.size);
  } else {
    EHLOG(INFO) << "pc must be java method written with assembly" << maple::endl;
    return _URC_CONTINUE_UNWIND;
  }
  EHTable ehTable(startPC, mLSDA, pc);
  ehTable.ScanExceptionTable(kSearchPhase, true, *unwindException);
  // If the stack-top java frame is abnormal, eh still calls __mpl_personality_v0 now because .cfi_personality.
  if (ehTable.results.unwindReason == _URC_HANDLER_FOUND && ehTable.results.landingPad) {
    MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(*unwindException);
    if (VLOG_IS_ON(eh)) {
      EHLOG(INFO) << "Catch Exception Succeeded : " <<
          reinterpret_cast<MObject*>(GetThrownObject(*unwindException))->GetClass()->GetName() << maple::endl;
    }
    SetRegisters(*unwindException, *context, ehTable.results);
    if (!exceptionHeader->caughtByJava) {
      free(GetThrownException(*unwindException));
    } // otherwise exception wrapper is freed in begin_catch
    return _URC_INSTALL_CONTEXT;
  } else {
    // control flow reaches here if top-stack java frame is abnormal
    return _URC_CONTINUE_UNWIND;
  }
}
#endif
} // extern "C"
} // maplert
