/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "mpl_sighandler.h"

#ifdef __unix__
#include <dlfcn.h>
#include <link.h>
#include <sys/time.h>
#endif

#include <cerrno>
#include <cstdlib>

#include "mpl_logging.h"
#include "mpl_stacktrace.h"

namespace maple {

std::map<int, SigHandler::FuncPtr> SigHandler::sig2callback =
#ifdef __unix__
    {{SIGINT, nullptr},  {SIGINT, nullptr},  {SIGTERM, nullptr}, {SIGSEGV, nullptr},
     {SIGBUS, nullptr},  {SIGABRT, nullptr}, {SIGILL, nullptr},  {SIGFPE, nullptr},
     {SIGXFSZ, nullptr}, {SIGUSR1, nullptr}, {SIGUSR2, nullptr}, {SIGALRM, nullptr}};
#else
    {};
#endif

std::array enabledByDefault{SIGTERM, SIGSEGV, SIGBUS, SIGABRT, SIGILL, SIGFPE, SIGXFSZ};

void SigHandler::Enable() {
  for (int signum : enabledByDefault) {
    EnableSig(signum);
  }
}

void SigHandler::EnableAll() {
#ifdef __unix__
  for (auto it = sig2callback.cbegin(); it != sig2callback.cend(); ++it) {
    SetSigaction(it->first, Handler);
  }
#else
  LogInfo::MapleLogger(kLlWarn) << "Sighandler : signal handler isn't implemented for non-unix os" << std::endl;
#endif
}

void SigHandler::EnableSig(int sig) {
#ifdef __unix__
  if (!IsSupportedSig(sig)) {
    LogInfo::MapleLogger(kLlWarn) << "SigHandler::enable(...) : signum " << sig << " isn't supported" << std::endl;
    return;
  }

  SetSigaction(sig, Handler);
#else
  LogInfo::MapleLogger(kLlWarn) << "Sighandler : signal handler isn't implemented for non-unix os" << std::endl;
#endif
}

void SigHandler::DisableAll() {
  for (auto it = sig2callback.cbegin(); it != sig2callback.cend(); ++it) {
    SetDefaultSigaction(it->first);
  }
}

void SigHandler::DisableSig(int sig) {
  if (!IsSupportedSig(sig)) {
    return;
  }

  SetDefaultSigaction(sig);
}

void SigHandler::SetTimer(int seconds) {
#ifdef __unix__
  struct itimerval timeValue {
    {seconds, 0}, {
      seconds, 0
    }
  };

  if (setitimer(ITIMER_REAL, &timeValue, nullptr) != 0) {
    LogInfo::MapleLogger(kLlErr) << "setitimer failed with " << errno << std::endl;
    exit(EXIT_FAILURE);
  }
#endif
}

void SigHandler::SetCallback(int sig, SigHandler::FuncPtr callback) {
  if (!IsSupportedSig(sig)) {
    LogInfo::MapleLogger(kLlWarn) << "SigHandler :" << sig << " isn't supported" << std::endl;
    return;
  }

  sig2callback[sig] = callback;
}

bool SigHandler::IsSupportedSig(int sig) {
  return sig2callback.count(sig) > 0;
}

static std::string SEGVcodename(int code) {
#ifdef __unix__
  switch (code) {
    case SEGV_MAPERR:
      return "Address not mapped";
    case SEGV_ACCERR:
      return "Invalid permissions";
    default:
      return "unknown SEGV code";
  }
#else
  return std::string();
#endif
}

static std::string BUScodename(int code) {
#ifdef __unux__
  switch (code) {
    case BUS_ADRALN:
      return "Invalid address alignment";
    case BUS_ADRERR:
      return "Non-existent physical address";
    case BUS_OBJERR:
      return "Object-specific hardware error";
    default:
      return "unknown BUS code";
  }
#else
  return std::string();
#endif
}

static std::string ILLcodename(int code) {
#ifdef __unix__
  switch (code) {
    case ILL_ILLOPC:
      return "Illegal opcode";
    case ILL_ILLOPN:
      return "Illegal operand";
    case ILL_ILLADR:
      return "Illegal addressing mode";
    case ILL_ILLTRP:
      return "Illegal trap";
    case ILL_PRVOPC:
      return "Privileged opcode";
    case ILL_PRVREG:
      return "Privileged register";
    case ILL_COPROC:
      return "Coprocessor error";
    case ILL_BADSTK:
      return "Internal stack error";
    default:
      return "unknown ILL code";
  }
#else
  return std::string();
#endif
}

static std::string FPEcodename(int code) noexcept {
#ifdef __unix__
  switch (code) {
    case FPE_INTDIV:
      return "Integer divide-by-zero";
    case FPE_INTOVF:
      return "Integer overflow";
    case FPE_FLTDIV:
      return "Floating point divide-by-zero";
    case FPE_FLTOVF:
      return "Floating point overflow";
    case FPE_FLTUND:
      return "Floating point underflow";
    case FPE_FLTRES:
      return "Floating point inexact result";
    case FPE_FLTINV:
      return "Invalid floating point operation";
    case FPE_FLTSUB:
      return "Subscript out of range";
    default:
      return "unknown FPE code";
  }
#else
  return std::string();
#endif
}

static std::string XFSZcodename(int code) {
#ifdef __unix__
  switch (code) {
    case SEGV_MAPERR:
      return "Address not mapped";
    case SEGV_ACCERR:
      return "Invalid permissions";
    default:
      return "unknown XFSZ code";
  }
#else
  return std::string();
#endif
}

static bool NeedDumpFaultingAddr(int sig) {
#ifdef __unix__
  return (sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGFPE || sig == SIGXFSZ);
#else
  return false;
#endif
}

static void DumpErrorMsg(int sig, int code) {
#ifdef __unix__
  LogInfo::MapleLogger(kLlErr) << "Program ends with ";
  switch (sig) {
    case SIGINT:
      LogInfo::MapleLogger(kLlErr) << "interrupt";
      break;
    case SIGTERM:
      LogInfo::MapleLogger(kLlErr) << "terminate()";
      break;
    case SIGSEGV:
      LogInfo::MapleLogger(kLlErr) << "SEGV signal: " << SEGVcodename(code);
      break;
    case SIGBUS:
      LogInfo::MapleLogger(kLlErr) << "BUS signal: " << BUScodename(code);
      break;
    case SIGABRT:
      LogInfo::MapleLogger(kLlErr) << "abort()";
      break;
    case SIGILL:
      LogInfo::MapleLogger(kLlErr) << "ILL signal: " << ILLcodename(code);
      break;
    case SIGFPE:
      LogInfo::MapleLogger(kLlErr) << "FPE signal: " << FPEcodename(code);
      break;
    case SIGXFSZ:
      LogInfo::MapleLogger(kLlErr) << "XFSZ signal: " << XFSZcodename(code);
      break;
    case SIGUSR1:
    case SIGUSR2:
      LogInfo::MapleLogger(kLlErr) << "USR signal";
      break;
    case SIGALRM:
      LogInfo::MapleLogger(kLlErr) << "ALRM signal";
      break;
    default:
      LogInfo::MapleLogger(kLlErr) << "unknown signal";
      break;
  }

  LogInfo::MapleLogger(kLlErr) << std::endl;
#endif
}

static uintptr_t GetLinkAddr(uintptr_t addr) {
#ifdef __unix__
  Dl_info info;
  void *extraInfo = nullptr;
  int status = dladdr1(reinterpret_cast<void *>(addr), &info, &extraInfo, static_cast<int>(RTLD_DL_LINKMAP));
  link_map *linkMap = static_cast<link_map *>(extraInfo);

  if (status != 1) {
    return 0;
  }

  return reinterpret_cast<uintptr_t>(linkMap->l_addr);
#else
  return 0;
#endif
}

static void InitAlternativeStack() {
#ifdef __unix__
  stack_t oldStack;
  if (sigaltstack(nullptr, &oldStack) == -1) {
    LogInfo::MapleLogger(kLlErr) << "sigaltstack failed with " << errno << std::endl;
    exit(EXIT_FAILURE);
  }

  if (oldStack.ss_sp) {
    return;
  }

  stack_t stack;
  constexpr size_t stackSizeMultiplier = 4;
  size_t stackSize = stackSizeMultiplier * SIGSTKSZ;
  stack.ss_size = stackSize;
  stack.ss_sp = std::malloc(stackSize);
  if (stack.ss_sp == nullptr) {
    LogInfo::MapleLogger(kLlErr) << "malloc failed with " << errno << std::endl;
    exit(EXIT_FAILURE);
  }

  stack.ss_flags = 0;

  if (sigaltstack(&stack, nullptr) == -1) {
    LogInfo::MapleLogger(kLlErr) << "sigaltstack faled with " << errno << std::endl;
    exit(EXIT_FAILURE);
  }
#else
  return;
#endif
}

void SigHandler::SetSigaction(int sig, SigHandler::FuncPtr callback) {
#ifdef __unix__
  InitAlternativeStack();

  struct sigaction sigact = {};
  sigact.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigact.sa_sigaction = callback;

  if (sigaction(sig, &sigact, nullptr) != 0) {
    LogInfo::MapleLogger(kLlErr) << "sigacton failed with " << errno << std::endl;
    exit(EXIT_FAILURE);
  }
#else
  return;
#endif
}

void SigHandler::SetDefaultSigaction(int sig) {
#ifdef __unix__
  struct sigaction sigact = {};
  sigact.sa_handler = SIG_DFL;
  if (sigaction(sig, &sigact, nullptr) != 0) {
    LogInfo::MapleLogger(kLlErr) << "sigacton failed with " << errno << std::endl;
    exit(EXIT_FAILURE);
  }
#else
  return;
#endif
}

void DumpFaultingAddr(uintptr_t addr) {
  uintptr_t linkAddr = GetLinkAddr(addr);
  LogInfo::MapleLogger(kLlErr) << "faulting address: ";
  if (linkAddr != 0) {
    LogInfo::MapleLogger(kLlErr) << "0x" << std::hex << addr - linkAddr << " (0x" << std::hex << addr << ")"
                                 << std::endl;
  } else {
    LogInfo::MapleLogger(kLlErr) << "0x" << std::hex << addr << std::endl;
  }
}

void SigHandler::Handler(int sig, siginfo_t *info, void *ucontext) noexcept {
  DumpErrorMsg(sig, info->si_code);

  if (NeedDumpFaultingAddr(sig)) {
    DumpFaultingAddr(reinterpret_cast<uintptr_t>(info->si_addr));
  }

  if (sig != SIGTERM) {
    LogInfo::MapleLogger(kLlErr) << Stacktrace<>() << std::endl;

    if (FuncPtr callback = sig2callback.at(sig)) {
      callback(sig, info, ucontext);
    }
  }

  exit(EXIT_FAILURE);
}

}  // namespace maple
