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
#include <sys/mman.h>
#include <csignal>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <map>
#include <dlfcn.h>
#include "libs.h"
#include "chelper.h"
#include "sizes.h" // runtime/include
#include "exception/mpl_exception.h"
#include "yieldpoint.h"
#include "exception_store.h"
#include "linker_api.h"
#include "thread_helper.h"
#include "mrt_common.h"
#include "exception/stack_unwinder.h"
#include "utils/time_utils.h"

namespace maplert {
//  Access a label defined in assembly from within C
//  1.  Export the label using .globl in the assembly
//  1.1 For _ZTI37Ljava_2Flang_2FArithmeticException_3B and
//      _ZTI38Ljava_2Flang_2FNullPointerException_3B,
//      the labels are defined in the same compilation unit
//      (see the below comment about a couple of dummy functions)
//      we don't need the step.
//  2. Declare that the labels are external ones.
//  3. Don't for get to take the ADDRESS of the symbol.
using namespace maple;

extern "C" ClassInitState MRT_TryInitClassOnDemand(const MClass &classinfo);
extern "C" bool MRT_ClassInitialized(jclass classinfo);

// Exception handling through signals
//
// Throwing an exception from a signal handler will not automatically notify the
// kernel that we have finished signal handling.  Therefore, the kernel still
// thinks we are handlign exceptions.  The kernel keeps the same signal blocked
// when we are handling it, and it will prevent a second SIGSEGV to be thrown
// when a hardware trap is triggered by a subsequent NULL pointer access.
//
// This function is a workaround.  It unblocks the SIGSEGV before throwing a
// Java exception.  This allows repeated NULL pointer accesses to throw NPE, but
// the correct implementation needs to actually look at the PC to find out if
// the SIGSEGV is actually caused by a Java instruction.  We also need to
// thoroughly investigate the side effects of rt_sigreturn (the function which
// is called after a signal handler returns) in addition to unblocking the
// signal and restoring the mcontext.
//
// PrepareToHandleJavaSignal implicitly does the job by RealHandler1 when signal
// stub frame is retreated.
using RealHandler2 = void (*)(void*, void*);
struct SignalHandler {
  // pc is utilized to restore control flow after signal is handled
  void *mPC;                  // the program point where this signal is raised
  void *mReturnPC;            // the return pc after this signal is handled
  RealHandler1 mHandler;      // the real signal handler
  void *mArg1;                // the first argument of mHandler
  RealHandler1 mResetHandler; // the page reset handler called after mHandler done
  void *mArgr1;               // the first argument of mResetHandler
};

// this is unreliable because sh is deleted after real signal handler is excecuted.
// we can not use this in HandleJavaSignalStub after calling InvokeJavaSignalHandler
// since sh is already deleted.
extern "C" void *GetRaisedSignalPC() {
  SignalHandler *sh = reinterpret_cast<SignalHandler*>(maple::ExceptionVisitor::GetSignalHandler());
  if (sh == nullptr) {
    LOG(FATAL) << "signale handler should not be nullptr" << maple::endl;
    return nullptr;
  }
  return sh->mPC;
}

extern "C" void MRT_InvokeResetHandler() {
  SignalHandler *sh = reinterpret_cast<SignalHandler*>(maple::ExceptionVisitor::GetSignalHandler());
  if (sh == nullptr) {
    return;
  }
  maple::ExceptionVisitor::SetSignalHandler(nullptr);
  RealHandler1 handler = sh->mResetHandler;
  if (handler != nullptr) {
    handler(sh->mArgr1);
  }
  delete sh;
  sh = nullptr;
}

extern "C" void *InvokeJavaSignalHandler() {
  auto sh = reinterpret_cast<SignalHandler*>(maple::ExceptionVisitor::GetSignalHandler());
  if (sh == nullptr) {
    LOG(FATAL) << "signale handler should not be nullptr" << maple::endl;
    return nullptr;
  }
  if (sh->mResetHandler == nullptr) {
    maple::ExceptionVisitor::SetSignalHandler(nullptr);
  }
  RealHandler1 handler = sh->mHandler;
  void *arg1 = sh->mArg1;
  void *returnPC = sh->mReturnPC;
  if (sh->mResetHandler == nullptr) {
    delete sh;
    sh = nullptr;
  }
  handler(arg1);
  return returnPC;
}

extern "C" void HandleJavaSignalStub();

// this is a stub only for handling signals raised in java methods
extern "C" bool MRT_PrepareToHandleJavaSignal(ucontext_t *ucontext,
                                              RealHandler1 handler, void *arg1,
                                              RealHandler1 rhandler, void *r1,
                                              int32_t returnPCOffset) {
  // For parameter returnPCOffset, its default valus is 1, that is pc + 4 , mean to issue the next instructions after
  // signale handling. but signals like lazybinding or decouple would like to redo the instruction bundle
  // after handling the signal. We can not set register x17 to the alctually return pc but pc + 4 because
  // the value in x17 will set to the eh frame which is used to determin if the EH is inside a try block.  So
  // here we set the x17 as (pc + 4) and sh->mReturnPC as pc + 4 * returnPCOffset.
  mcontext_t *mcontext = &(ucontext->uc_mcontext);

  auto sh = new (std::nothrow) SignalHandler;
  if (sh == nullptr) {
    LOG(FATAL) << "new SignalHandler failed" << maple::endl;
  }
  sh->mHandler = handler;
  sh->mArg1 = arg1;
  sh->mPC = reinterpret_cast<void*>(mcontext->pc);
  sh->mReturnPC = reinterpret_cast<void*>(mcontext->pc + kAarch64InsnSize * returnPCOffset);
  sh->mResetHandler = rhandler;
  sh->mArgr1 = r1;
  ExceptionVisitor::SetSignalHandler(sh);

  {
    // signal handler frame is specifically structured, and can not be uniwinded according to ABI, which is more or less
    // like the frames constructed by third-party native code, thus we have to update this java context as *risky*.
    MRT_UpdateLastUnwindContext(reinterpret_cast<uint32_t*>(mcontext->pc + kAarch64InsnSize),
                                reinterpret_cast<CallChain*>(mcontext->regs[kFP]), UnwindContextIsRisky);
    // make lr valid for java method without an frame.
    if (JavaFrame::JavaMethodHasNoFrame(reinterpret_cast<const uint32_t*>(mcontext->pc))) {
      MRT_UpdateLastUnwindFrameLR(reinterpret_cast<const uint32_t*>(mcontext->regs[kLR]));
    }
  }

  // backup the pc in the abnormal frame to x17.
  // set x17 to "pc + 4", and mimic as if this is a bl instruction.
  // Note _Unwind_GetIP relies on this value to get
  // the context where an exception is raised. __mpl_personality_v0 should cooperate
  // with this backup. otherwise, search exception table will fail.
  //
  // Furthermore, _Unwind APIs assume the value of pc points to the next instruction
  // of the instruction which raises a signal. Otherwise stack unwinding fails when
  // the first instruction of a method in an abnormal frame raises a signal.
  // refer to uw_update_context in libgcc.
  mcontext->regs[kX17] = mcontext->pc + kAarch64InsnSize;

  // mimic the return address for HandleJavaSignalStub, this is necessary for gdb
  // as well as _Unwind_xx APIs, since control flow *jumps* to HandleJavaSignalStub,
  // and does not save exceptioning instruction pointer automatically.
  // We need a precise site where raises an exception for _Unwind_xx APIs to work.
  // backup x30 is an alternative option.
  mcontext->pc = reinterpret_cast<uint64_t>(&HandleJavaSignalStub); // ready to perform signal handling
  return true;
}

static int UnblockSignal(int sig) {
  sigset_t sigSet;
  if (UNLIKELY(sigemptyset(&sigSet) == -1)) {
    LOG(ERROR) << "sigemptyset() in UnblockSignal() return -1 for failed." << maple::endl;
  }
  if (UNLIKELY(sigaddset(&sigSet, sig) == -1)) {
    LOG(ERROR) << "sigaddset() in UnblockSignal() return -1 for failed." << maple::endl;
  }
  sigset_t oldSet;
  int err = sigprocmask(SIG_UNBLOCK, &sigSet, &oldSet);
  if (err != 0) {
    PLOG(ERROR) << "UnblockSignal failed" << maple::endl;
  }
  return err;
}

static void *AnalyzeLazyloadInstructionSequence(uint32_t *pc4, ucontext_t *ucontext) {
  uint32_t *pc0 =  pc4 - 1;
  // 0:         ldr   wm [xn] // xm for 64bit
  // 4:         ldr   wm [wm]
#ifdef USE_32BIT_REF
  constexpr static uint32_t prefixCode0 = 0xb9400;
#else
  constexpr static uint32_t prefixCode0 = 0xf9400;
#endif
  constexpr static uint32_t prefixCode4 = 0xb9400;
  constexpr static uint32_t regMask = 0b11111;
  constexpr static uint32_t kMplOffsetFixFlag = 0x80000000;
  constexpr static uint16_t regCodeOffset = 12;
  constexpr static uint16_t regNumOffset = 5;

  uint32_t code0 = (*pc0 >> regCodeOffset);
  uint32_t code4 = (*pc4 >> regCodeOffset);
  uint32_t distRegNum0 = *pc0 & regMask;
  uint32_t srcRegNum0 =  (*pc0 >> regNumOffset) & regMask;
  uint32_t srcRegNum4 =  (*pc4 >> regNumOffset) & regMask;

  if (code0 == prefixCode0 && code4 == prefixCode4 && distRegNum0 != srcRegNum0 &&
      distRegNum0 == srcRegNum4 && ucontext != nullptr) {
    // Further check the LazyLoad Sentry Number
    LinkerOffsetValItemLazyLoad *offValTabAddr =
        reinterpret_cast<LinkerOffsetValItemLazyLoad*>(ucontext->uc_mcontext.regs[srcRegNum0]);
    uint32_t offsetValue = offValTabAddr->offset;
    if (offsetValue & kMplOffsetFixFlag) {
      uint32_t offsetIndex = (offsetValue & (~kMplOffsetFixFlag));
      if (*(reinterpret_cast<uint64_t*>(offValTabAddr - offsetIndex) - 1) != kMplLazyLoadSentryNumber) {
        return nullptr;
      }
    }
    return offValTabAddr;
  }
  return nullptr;
}

const uint64_t kAdrpBinaryCode = 0b10010000;
const uint64_t kLdrBinaryCode  = 0b1011100101;
const uint64_t kAdrBinaryCode  = 0b00010000;
const uint64_t kAddBinaryCode  = 0b00010001;
const uint64_t kShortCmdMask   = 0b10011111;
const uint64_t kLongCmdMask    = 0b1011111111;
const uint64_t kLdrXtMask      = 0b11111;
const uint64_t kXzrReg         = 31;
const uint32_t kLongCmdMaskOffset = 22;
const uint32_t kShortCmdMaskOffset = 24;

uint64_t *PICAdrpCodeCheck(const uint32_t *pc) {
  // case 1.1: PIC code
  //    0: 0x90012c20  adrp  x0, 0x555a844000          # (code >> 24) &   0b10011111 ==   0b10010000
  //    4: 0xf9400000  ldr   x0, [x0,#1024]            # (code >> 22) & 0b1011111111 == 0b1011100101
  //    8: 0xf9400000  ldr   x0, [x0,#112]             # (code >> 22) & 0b1011111111 == 0b1011100101
  //   12: 0xf9400000  ldr   x0, [x0]                  # (code >> 22) & 0b1011111111 == 0b1011100101
  if ((((*(pc + 0) >> kShortCmdMaskOffset) & kShortCmdMask) == kAdrpBinaryCode) &&
      (((*(pc + 1) >> kLongCmdMaskOffset) & kLongCmdMask) == kLdrBinaryCode)) {
    {
      // decode adrp (refer to ARM Architecture Reference Manual ARMv8 C6.2.10)
      uintptr_t adrpCode = *pc;
      // 19 + 2 + 12 bits
      uintptr_t adrpOffset = ((((adrpCode >> 5) & 0x7FFFF) << 2) | ((adrpCode >> 29) & 0b11)) << 12;
      // 19 + 2 + 12 bits sign-extend to 64 bits
      adrpOffset = (adrpOffset << (64 - 33)) >> (64 - 33);
      // relative to pc page address.
      uintptr_t targetPage = ((reinterpret_cast<uintptr_t>(pc) >> 12) << 12) + adrpOffset;

      // decode add (refer to ARM Architecture Reference Manual ARMv8 C6.2.4)
      uint32_t ldrCode = *(pc + 1);
      uintptr_t ldrOffset = (ldrCode >> 10) & 0xFFF; // 12 bits
      uint32_t ldrScale = (ldrCode >> 30) & 0b11; // 2 bits
      return reinterpret_cast<uint64_t*>(targetPage + (ldrOffset << ldrScale));
    }
  }
  return nullptr;
}

uint64_t *PICAdrCodeCheck(const uint32_t *pc, uint32_t secondLdrOffset) {
  // case 1.2: PIC code. CG does not generate these code, but assembler will replace adrp for adr
  // 0: 0x90012c20  adr  x0, 0xffff7030d000          # (code >> 24) &   0b10011111 ==   0b10010000
  // 4: 0xf9400000  ldr   x0, [x0,#1024]            # (code >> 22) & 0b1011111111 == 0b1011100101
  // 8: 0xf9400000  ldr   x0, [x0,#112]             # (code >> 22) & 0b1011111111 == 0b1011100101
  // 12: 0xf9400000  ldr   x0, [x0]                  # (code >> 22) & 0b1011111111 == 0b1011100101
  if ((((*(pc + 0) >> kShortCmdMaskOffset) & kShortCmdMask) == kAdrBinaryCode) &&
      (((*(pc + 1) >> kLongCmdMaskOffset) & kLongCmdMask) == kLdrBinaryCode) &&
      (((*(pc + secondLdrOffset) >> kLongCmdMaskOffset) & kLongCmdMask) == kLdrBinaryCode)) {
    {
      // decode adrp (refer to ARM Architecture Reference Manual ARMv8 C6.2.10)
      uintptr_t adrCode = *pc;
      // 19 + 2 bits
      uintptr_t adrOffset = ((((adrCode >> 5) & 0x7FFFF) << 2) | ((adrCode >> 29) & 0b11));
      // 19 + 2 bits sign-extend to 64 bits
      adrOffset = (adrOffset << (64 - 21)) >> (64 - 21);
      // relative to pc address.
      uintptr_t targetPage = reinterpret_cast<uintptr_t>(pc) + adrOffset;

      // decode add (refer to ARM Architecture Reference Manual ARMv8 C6.2.4)
      uint32_t ldrCode = *(pc + 1);
      uintptr_t ldrOffset = (ldrCode >> 10) & 0xFFF; // 12 bits
      uint32_t ldrScale = (ldrCode >> 30) & 0b11; // 2 bits
      return reinterpret_cast<uint64_t*>(targetPage + (ldrOffset << ldrScale));
    }
  }
  return nullptr;
}

uint64_t *NonPICCodeCheck(const uint32_t *pc) {
  // case 2: non-PIC code
  //    0: 0x90012c20  adrp  x0, 0x555a844000          # (code >> 24) &   0b10011111 ==   0b10010000
  //    4: 0x911e2000  add   x0, x0, #0x788            # (code >> 24) &   0b01111111 ==   0b00010001
  //    8: 0xf9400000  ldr   x0, [x0,#112]             # (code >> 22) & 0b1011111111 == 0b1011100101
  //   12: 0xf9400000  ldr   x0, [x0]                  # (code >> 22) & 0b1011111111 == 0b1011100101
  if ((((*(pc + 0) >> kShortCmdMaskOffset) & kShortCmdMask) == kAdrpBinaryCode) &&
      (((*(pc + 1) >> kShortCmdMaskOffset) & kShortCmdMask) == kAddBinaryCode)) {
    {
      // decode adrp (refer to ARM Architecture Reference Manual ARMv8 C6.2.10)
      uintptr_t adrpCode = *pc;

      // 19 + 2 + 12 bits
      uintptr_t adrpOffset = ((((adrpCode >> 5) & 0x7FFFF) << 2) | ((adrpCode >> 29) & 0b11)) << 12;
      // 19 + 2 + 12 bits sign-extend to 64 bits
      adrpOffset = (adrpOffset << (64 - 33)) >> (64 - 33);
      // relative to pc page address.
      uintptr_t targetPage = ((reinterpret_cast<uintptr_t>(pc) >> 12) << 12) + adrpOffset;

      // decode add (refer to ARM Architecture Reference Manual ARMv8 C6.2.4)
      uint32_t addCode = *(pc + 1);
      uintptr_t addOffset = (addCode >> 10) & 0xFFF; // 12 bits
      uint32_t addShift = (addCode >> 22) & 0b11; // 2 bits
      __MRT_ASSERT(addShift < 0b10, "AnalyzeClinitInstructionSequence addShift error");
      if (addShift == 0b01) {
        addOffset <<= 12;
      }
      return reinterpret_cast<uint64_t*>(targetPage + addOffset);
    }
  }
  return nullptr;
}

// analyze following instructions:
static uint64_t *AnalyzeClinitInstructionSequence(const uint32_t *pc) {
  uint32_t secondLdrOffset = 2;
  // check the case with a lazybind instruction
  // -4: 0xd0000080  adrp x0,  14000 <_GLOBAL_OFFSET_TABLE_+0xf0>
  // 0: 0xf9425000  ldr  x0,  [x0,#1184]
  // 4: 0xf940001f  ldr  xzr, [x0]    // for lazy binnding
  // 8: 0xf9401811  ldr  x17, [x0,#48]
  // c: 0xf940023f  ldr  xzr, [x17]
  if ((((*(pc + 0) >> kLongCmdMaskOffset) & kLongCmdMask) == kLdrBinaryCode) &&
      (((*(pc + 1) >> kLongCmdMaskOffset) & kLongCmdMask) == kLdrBinaryCode) &&
      ((*(pc + 1) & kLdrXtMask) == kXzrReg)) {
    --pc;
    ++secondLdrOffset;
  }

  uint64_t *result = nullptr;

  result = PICAdrpCodeCheck(pc);
  if (result != nullptr) {
    return result;
  }

  result = PICAdrCodeCheck(pc, secondLdrOffset);
  if (result != nullptr) {
    return result;
  }

  result = NonPICCodeCheck(pc);
  if (result != nullptr) {
    return result;
  }

  return nullptr;
}

static int64_t sigvCausedClinit = 0;

static inline int64_t IncSigvCausedClinitCount(int64_t count) {
  return __atomic_add_fetch(&sigvCausedClinit, count, __ATOMIC_ACQ_REL);
}

namespace {
constexpr uint16_t kAdrpPcOffsetForLazy = 2;
}

static inline uint32_t GetLdrSrcRegNum(uint32_t pc) {
  constexpr static uint16_t srcRegNumOffset = 5;
  constexpr static uint32_t regMask = 0b11111;
  return (pc >> srcRegNumOffset) & regMask;
}

static bool AnalyzeLoadArrayCacheInstructionSequence(uint32_t *pc, ucontext_t &ucontext) {
  // adrp    x1, 1798000
  // ldr     w1, [x1,#2400]
  // ldr     wzr, [x1]
  constexpr uint16_t adrpPCOffsetForArrayCache = 2;
  uint32_t *adrpPC = pc - adrpPCOffsetForArrayCache;
  uint64_t *addr = AnalyzeClinitInstructionSequence(adrpPC);
  if (addr == nullptr) {
    return MRT_PrepareToHandleJavaSignal(&ucontext, &MRT_ThrowImplicitNullPointerExceptionUnw, pc);
  }

  SignalInfo *pSignalInfo = new (std::nothrow) SignalInfo(pc, addr);
  MplCheck(pSignalInfo != nullptr, "pSignalInfo is nullptr");
  constexpr int32_t arrayCacheReturnPCOffset = -2;
  return MRT_PrepareToHandleJavaSignal(&ucontext, reinterpret_cast<RealHandler1>(MRT_RequestInitArrayCache),
                                       pSignalInfo, nullptr, nullptr, arrayCacheReturnPCOffset);
}

static bool HandleStaticDecoupleSignal(ucontext_t &ucontext) {
  uint32_t *pc = reinterpret_cast<uint32_t*>(ucontext.uc_mcontext.pc);
  constexpr int32_t kDecoupleStaticReturnPCOffset = -2;

  auto *item = reinterpret_cast<LinkerStaticAddrItem*>(
      AnalyzeClinitInstructionSequence(pc - kAdrpPcOffsetForLazy));
  if (item == nullptr) {
    return MRT_PrepareToHandleJavaSignal(&ucontext, &MRT_ThrowImplicitNullPointerExceptionUnw, pc);
  }
  MplStaticAddrTabHead *head = reinterpret_cast<MplStaticAddrTabHead*>(item - item->index) - 1;
  if (head->magic == kMplStaticLazyLoadSentryNumber) {
    int32_t returnPCOffset = kDecoupleStaticReturnPCOffset;
    if (item->dcpAddr != 0) { // kMplStaticItemUnResolved
      ucontext.uc_mcontext.regs[GetLdrSrcRegNum(*pc)] = item->dcpAddr; // this solves the static call in <clinit>
      returnPCOffset = 1;
    }

    if (item->address == kMplLazyStaticDecoupleMagicNumber) {
      return MRT_PrepareToHandleJavaSignal(&ucontext,
                                           reinterpret_cast<RealHandler1>(MRT_FixStaticAddrTableLazily),
                                           item, nullptr, nullptr, returnPCOffset);
    } else {
      return MRT_PrepareToHandleJavaSignal(&ucontext,
                                           reinterpret_cast<RealHandler1>(MRT_FixStaticAddrTable),
                                           item, nullptr, nullptr, returnPCOffset);
    }
  } else {
    return MRT_PrepareToHandleJavaSignal(&ucontext, &MRT_ThrowImplicitNullPointerExceptionUnw, pc);
  }
}

static bool HandleSigsegvFromJava(siginfo_t *info, ucontext_t *ucontext) {
  constexpr uint16_t adrpPcOffsetForClinit = 3;
  constexpr int32_t kLazyBindingReturnPcOffset = -2;
  constexpr int32_t kDecoupleReturnPcOffset = -1;

  // must unblock SIGSEGV before calling <clinit>
  int tmpResult = UnblockSignal(SIGSEGV);
  if (UNLIKELY(tmpResult != 0)) {
    LOG(ERROR) << "UnblockSignal in HandleSigsegvFromJava() return " << tmpResult << " rather than 0." <<
        maple::endl;
  }
  if (ucontext == nullptr) {
    LOG(FATAL) << "ucontext is nullptr, lead to crash." << maple::endl;
  }

  mcontext_t *mcontext = &(ucontext->uc_mcontext);
  uint32_t *pc = reinterpret_cast<uint32_t*>(mcontext->pc);

  if (reinterpret_cast<uintptr_t>(info->si_addr) == static_cast<uintptr_t>(kSEGVAddrForClassUninitialized) ||
      reinterpret_cast<uintptr_t>(info->si_addr) == static_cast<uintptr_t>(kSEGVAddrForClassInitializing)  ||
      info->si_addr == &classInitProtectRegion[static_cast<int>(kClassUninitialized) - 1]) {
    // case 1: segv triggered by class init
    uint32_t *adrpPc = pc - adrpPcOffsetForClinit;
    int64_t sigvCausedClinitCount = IncSigvCausedClinitCount(1);
    VLOG(classinit) << "clinit segv " << sigvCausedClinit << std::endl;
    VLOG(classinit) << "sigvCausedClinitCount" << sigvCausedClinitCount << std::endl;
#ifdef USE_32BIT_REF
    void *classinfo = MRT_GetAddress32ByAddress(AnalyzeClinitInstructionSequence(adrpPc));
#else
    void *classinfo = MRT_GetAddressByAddress(AnalyzeClinitInstructionSequence(adrpPc));
#endif // USE_32BIT_REF
    if (classinfo == nullptr) {
      maplert::MRT_Panic();
    }
    return MRT_PrepareToHandleJavaSignal(ucontext, (RealHandler1)&MRT_TryInitClassOnDemand, classinfo);
  } else if (reinterpret_cast<uintptr_t>(info->si_addr) == static_cast<uintptr_t>(kSEGVAddrForClassInitFailed)) {
    // case 2: segv triggered by class init failure
    uint32_t *adrpPc = pc - adrpPcOffsetForClinit;
    int64_t sigvCausedClinitCount = IncSigvCausedClinitCount(1);
    VLOG(classinit) << "clinit segv " << sigvCausedClinit << std::endl;
    VLOG(classinit) << "sigvCausedClinitCount" << sigvCausedClinitCount << std::endl;
#ifdef USE_32BIT_REF
    void *classinfo = MRT_GetAddress32ByAddress(AnalyzeClinitInstructionSequence(adrpPc));
#else
    void *classinfo = MRT_GetAddressByAddress(AnalyzeClinitInstructionSequence(adrpPc));
#endif // USE_32BIT_REF
    char msg[kBufferSize] = { 0 };
    if (sprintf_s(msg, sizeof(msg), "unexpected class initialization failure for classinfo encoded at %p",
                  adrpPc) < 0) {
      LOG(ERROR) << "HandleSigsegvFromJava sprintf_s fail" << maple::endl;
    }
    MplCheck(classinfo != nullptr, msg);
    // this will actually invoke MRT_ThrowNoClassDefFoundErrorClassUnw(classinfo)
    return MRT_PrepareToHandleJavaSignal(ucontext,
                                         &MRT_ThrowNoClassDefFoundErrorClassUnw,
                                         reinterpret_cast<ClassMetadata*>(classinfo));
#ifdef LINKER_LAZY_BINDING
  } else if (info->si_addr >= __BindingProtectRegion__ &&
             info->si_addr < &__BindingProtectRegion__[kBindingStateMax]) {
    // SEGV triggered by Linker binding.
    VLOG(lazybinding) << "HandleSigsegvFromJava(), info->si_addr=" << info->si_addr <<
        ", between {" << reinterpret_cast<void*>(__BindingProtectRegion__) <<
        ", " << reinterpret_cast<void*>(&__BindingProtectRegion__[kBindingStateMax]) << "}" << maple::endl;
    uint32_t *adrpPc = pc - kAdrpPcOffsetForLazy;
    void *addr = reinterpret_cast<void*>(AnalyzeClinitInstructionSequence(adrpPc));
    SignalInfo *pSignalInfo = new (std::nothrow) SignalInfo(pc, addr);
    if (pSignalInfo == nullptr) {
      LOG(FATAL) << "new SignalInfo failed" << maple::endl;
    }
    return MRT_PrepareToHandleJavaSignal(ucontext, reinterpret_cast<RealHandler1>(MRT_RequestLazyBindingForSignal),
                                         pSignalInfo, nullptr, nullptr, kLazyBindingReturnPcOffset);
#endif // LINKER_LAZY_BINDING
  } else if (reinterpret_cast<uintptr_t>(info->si_addr) == kMplLazyLoadMagicNumber) {
    // segv triggerd by lazy load
    // 0:         ldr   wm [xn] // xm for 64bit
    // 4:         ldr   wm [wm]
    void *offsetEntry = AnalyzeLazyloadInstructionSequence(pc, ucontext);
    if (offsetEntry != nullptr) {
      return MRT_PrepareToHandleJavaSignal(ucontext, reinterpret_cast<RealHandler1>(MRT_FixOffsetTableLazily),
                                           offsetEntry, nullptr, nullptr, kDecoupleReturnPcOffset);
    } else {
      return MRT_PrepareToHandleJavaSignal(ucontext, &MRT_ThrowImplicitNullPointerExceptionUnw, pc);
    }
  } else if (reinterpret_cast<uintptr_t>(info->si_addr) == kMplLazyStaticDecoupleMagicNumber ||
             reinterpret_cast<uintptr_t>(info->si_addr) == kMplStaticDecoupleMagicNumber) {
    return HandleStaticDecoupleSignal(*ucontext);
  } else if (reinterpret_cast<uintptr_t>(info->si_addr) == kMplArrayClassCacheMagicNumber) {
    return AnalyzeLoadArrayCacheInstructionSequence(pc, *ucontext);
  } else if (reinterpret_cast<uintptr_t>(info->si_addr) < static_cast<uintptr_t>(maple::kPageSize)) {
    // case 3: segv triggered by NPE
    // this will actually invoke MRT_ThrowImplicitNullPointerExceptionUnw(pc)
    // if not null may have compiler
    if (info->si_addr != nullptr) {
      LOG(ERROR) << " info->si_addr not null: " << std::hex << info->si_addr << " PC: " <<
          std::hex << mcontext->pc << maple::endl;
    }
    return MRT_PrepareToHandleJavaSignal(ucontext, &MRT_ThrowImplicitNullPointerExceptionUnw, pc);
  }
  return false;
}

// Signal handler For Android, return value matters.
extern "C" bool MRT_FaultHandler(int sig, siginfo_t *info, ucontext_t *ucontext, bool isFromJava) {
  auto origErrno = errno;

  if (ucontext == nullptr) {
    LOG(FATAL) << "ucontext is nullptr, lead to crash." << maple::endl;
  }
  if (isFromJava) {
    // signal handler is some kind of java-to-runtime stub frame, so we need to update the unwind context
    mcontext_t *mcontext = &(ucontext->uc_mcontext);
    MRT_UpdateLastUnwindContextIfReliable(reinterpret_cast<uint32_t*>(mcontext->pc + kAarch64InsnSize),
                                          reinterpret_cast<void*>(mcontext->regs[kFP] /* FP */));
  }

  if (YieldpointSignalHandler(sig, info, ucontext)) {
    errno = origErrno;
    return true;
  } else if (UNLIKELY(ThreadHelper::SuspendHandler())) {
    timeutils::SleepForever();
  } else if (isFromJava) {
    bool ret = false;
    switch (sig) {
      case SIGSEGV:
        ret = HandleSigsegvFromJava(info, ucontext);
        break;
      default: {
        LOG(ERROR) << "maple runtime internal error: unexpected signal from java frame." << maple::endl;
      }
    }
    return ret;
  }

  errno = origErrno;
  return false;
}

// siginfo_t* info for future extention
extern "C" bool MRT_FaultDebugHandler(int sig, siginfo_t *info __attribute__((unused)), void *context) {
  if (context == nullptr) {
    LOG(FATAL) << "context is nullptr, lead to crash." << maple::endl;
  }
  ucontext_t *ucontext = reinterpret_cast<ucontext_t*>(context);
  mcontext_t *mcontext = &(ucontext->uc_mcontext);
  switch (sig) {
    case SIGTRAP: {
      // x0 is addr of obj
      JsanliteError(mcontext->regs[0]);
      break;
    }
    default: {
      break;
    }
  }
  return false;
}
}
