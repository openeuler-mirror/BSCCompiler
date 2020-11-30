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
#include "exception/stack_unwinder.h"

#include <cstring>
#include <cstdint>
#include <cinttypes>
#include <ucontext.h>
#include <pthread.h>
#include <dlfcn.h>

#include "libs.h"
#include "mm_config.h"
#include "panic.h"
#include "syscall.h"
#include "exception_store.h"
#include "exception/mpl_exception.h"
#include "chosen.h"
#include "collie.h"
#include "mutator_list.h"
#include "interp_support.h"

namespace maplert {
const int kBuffSize = 24;
static RemoteUnwinder remoteUnwinder;

#define MAPLE_RECORD_METHOD_INFO_AT_START_PROC

// analyze the instruction:
//  .word __methods__Lexcp01802_3B+56-.
// pc belongs to section .text.java
//   desc - pc
// desc belongs to section .rodata
//   method info - desc
// method info belongs to .data
// using uintptr_t as offset means we should put section .rodata after section .text.java, thus use its signed
// counter-part.
const MethodDesc *JavaFrame::GetMethodDesc(const uint32_t *startPC) {
  const uint32_t *offsetAddr = startPC - kMethodDescOffset;
  const MethodDesc *methodDesc = reinterpret_cast<const MethodDesc*>(
      reinterpret_cast<uintptr_t>(offsetAddr) + static_cast<uintptr_t>(*offsetAddr));
  return methodDesc;
}

uint64_t *JavaFrame::GetMethodMetadata(const uint32_t *startPC) {
  const MethodDesc *methodDesc = GetMethodDesc(startPC);
  const uint32_t *offsetAddr = &(methodDesc->metadataOffset);
  uint64_t *metadata = reinterpret_cast<uint64_t*>(
      reinterpret_cast<uintptr_t>(offsetAddr) + static_cast<uintptr_t>(*offsetAddr));
  return metadata;
}

jclass JavaFrame::GetDeclaringClass(const uint64_t *md) {
  if (md != nullptr) {
    const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
    void *cls = methodMeta->GetDeclaringClass();
    if (cls == nullptr) {
      LOG(FATAL) << "class should not be null" << maple::endl;
    }
    // some faked compact methods don't have a valid declaringClass
    // for e.g., the methods in duplicateFunc.s and hashcode.s
    if (MRT_ReflectIsClass(reinterpret_cast<jobject>(cls))) {
      return reinterpret_cast<jclass>(cls);
    }
  }
  return nullptr;
}

// class name mangled by maple
void JavaFrame::GetMapleClassName(std::string &name, const uint64_t *md) {
  if (md != nullptr) {
    jclass cls = GetDeclaringClass(md);
    if (cls != nullptr) {
      name.append(reinterpret_cast<MClass*>(cls)->GetName());
    } else {
      name.append("figo.internal.class");
    }
  }
}

void JavaFrame::GetJavaClassName(std::string &name, const uint64_t *md) {
  if (md != nullptr) {
    jclass cls = GetDeclaringClass(md);
    if (cls) {
      reinterpret_cast<MClass*>(cls)->GetBinaryName(name);
    } else {
      name.append("figo.internal.class");
    }
  }
}

// fill in the string of method and type name
void JavaFrame::GetJavaMethodSignatureName(std::string &name, const void *md) {
  if (md != nullptr) {
    const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
    const std::string mathodName = methodMeta->GetName();
    name.append(mathodName);
  }
}

// fill in the string of method
void JavaFrame::GetJavaMethodName(std::string &name, const void *md) {
  if (md != nullptr) {
    const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
    name = methodMeta->GetName();
  }
}

bool JavaFrame::GetMapleMethodFullName(std::string &name, const uint64_t *md) {
  if (md != nullptr) {
    GetJavaClassName(name, md);
    const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
    const std::string methodName = methodMeta->GetName();
    std::string sigName;
    methodMeta->GetSignature(sigName);
    name.append("|").append(methodName).append("|").append(sigName);
    return true;
  }
  return false;
}

// <class-name>|<method-name>|([Ljava/lang/String;)V
bool JavaFrame::GetMapleMethodFullName(std::string &name, const uint64_t *md, const void *ip) {
  if (GetMapleMethodFullName(name, md)) {
    return true;
  } else {
    Dl_info addrInfo;
    int dladdrOk = dladdr(ip, &addrInfo);
    if (dladdrOk && addrInfo.dli_sname) {
      name.append(addrInfo.dli_sname);
      EHLOG(ERROR) << "Method meta data is invalid " << reinterpret_cast<uint64_t>(md) <<
          " for " << addrInfo.dli_sname << " at PC " << reinterpret_cast<const void*>(ip) << maple::endl;
      return true;
    }
  }
  return false;
}

// <class-name>.<method-name>([Ljava/lang/String;)V
bool JavaFrame::GetJavaMethodFullName(std::string &name, const uint64_t *md) {
  if (md != nullptr) {
    GetJavaClassName(name, md);
    const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
    const std::string mathodName = methodMeta->GetName();
    std::string sigName;
    methodMeta->GetSignature(sigName);
    name.append(".").append(mathodName).append(sigName);
    return true;
  }
  return false;
}

static bool DumpProcInfoForJava(
    uintptr_t pc, const LinkerLocInfo &procInfo, const LinkerMFileInfo &mplInfo, std::string *symbolInfo) {
  string methodName;
  const uint64_t *md = JavaFrame::GetMethodMetadata(reinterpret_cast<uint32_t*>(procInfo.addr));
  if (UNLIKELY(!JavaFrame::GetJavaMethodFullName(methodName, md))) {
    EHLOG(ERROR) << "JavaFrame::GetJavaMethodFullName() in DumpProcInfoForJava() return false." << maple::endl;
    return false;
  }
  uintptr_t lineNumber = pc - 1 - reinterpret_cast<uintptr_t>(procInfo.addr);

  if (symbolInfo != nullptr) {
    char ipBuf[kBuffSize] = {0};
    if (UNLIKELY(sprintf_s(ipBuf, sizeof(ipBuf), "0x%llx", static_cast<long long unsigned>(pc)) == -1)) {
      EHLOG(ERROR) << "sprintf_s() in DumpProcInfoForJava() return -1 for failed." << maple::endl;
      return false;
    }
    symbolInfo->append("\t").append(ipBuf).append(" in ").append(methodName).append(" at ").append(
        mplInfo.name).append(":").append(std::to_string(lineNumber)).append("\n");
  } else {
    EHLOG(INFO) << "\t" << std::hex << "0x" << pc << " in " << methodName <<
        " at " << mplInfo.name << ":" << std::dec << lineNumber << maple::endl;
  }
  return true;
}

uint64_t JavaFrame::GetRelativePc(const std::string &soName, uint64_t pc, LinkerMFileInfo *mplInfo) {
  if (mplInfo == nullptr) {
    mplInfo = LinkerAPI::Instance().GetLinkerMFileInfoByName(soName);
  }
  if (mplInfo == nullptr) {
    LOG(ERROR) << "mplInfo is nullptr" << maple::endl;
    return 0;
  }

  if (mplInfo->elfBase == nullptr) {
    (void)LinkerAPI::Instance().CheckLinkerMFileInfoElfBase(*mplInfo);
  }
  return pc - reinterpret_cast<uintptr_t>(mplInfo->elfBase);
}

void JavaFrame::DumpProcInfoLog(jlong elem) {
  uint64_t ip = static_cast<uint64_t>(elem);
  LinkerLocInfo info;
  LinkerMFileInfo *mplInfo = nullptr;
  bool isJava = LinkerAPI::Instance().GetJavaTextInfo(reinterpret_cast<uint32_t*>(ip), &mplInfo, info, false);
  if (isJava) {
    uint64_t *md = JavaFrame::GetMethodMetadata(reinterpret_cast<uint32_t*>(info.addr));
    std::string declaringClass;
    std::string methodName;
    std::string fileName;
    JavaFrame::GetJavaClassName(declaringClass, md);
    JavaFrame::GetJavaMethodSignatureName(methodName, md);
    fileName = mplInfo->name;
    uint64_t lineNumber = JavaFrame::GetRelativePc(fileName, static_cast<uintptr_t>(ip), mplInfo);
    EHLOG(INFO) << declaringClass << ":" << methodName << ":" << fileName << ":" << lineNumber << maple::endl;
  } else if (mplInfo != nullptr) {
    std::string fileName = mplInfo->name;
    uint64_t lineNumber = JavaFrame::GetRelativePc(fileName, static_cast<uintptr_t>(ip), mplInfo);
    EHLOG(ERROR) << fileName << ":" << lineNumber << maple::endl;
  } else {
    EHLOG(ERROR) << "Could not get mplinfo." << maple::endl;
  }
}

// use procInfo first if it is java code, then pc
bool JavaFrame::DumpProcInfo(uintptr_t pc, std::string *symbolInfo) {
  LinkerLocInfo info;
  LinkerMFileInfo *mplInfo = nullptr;
  bool isJava = LinkerAPI::Instance().GetJavaTextInfo(reinterpret_cast<uint32_t*>(pc), &mplInfo, info, false);
  if (isJava) {
    return DumpProcInfoForJava(pc, info, *mplInfo, symbolInfo);
  } else {
    Dl_info addrInfo;
    int dladdrOk = dladdr(reinterpret_cast<void*>(pc), &addrInfo);
    if (dladdrOk && addrInfo.dli_sname) {
      std::string fileName(addrInfo.dli_fname);
      uintptr_t lineNumber = pc - reinterpret_cast<uintptr_t>(addrInfo.dli_saddr);
      EHLOG(INFO) << "\t" << std::hex << "0x" << pc << " in " << addrInfo.dli_sname <<
          " at " << fileName << ":" << std::dec << lineNumber << maple::endl;
    } else {
      EHLOG(ERROR) << "\t" << std::hex << "0x" << pc << " in (unresolved symbol): dladdr() failed" <<
          std::dec << maple::endl;
      return false;
    }
  }
  return true;
}

bool JavaFrame::JavaMethodHasNoFrame(const uint32_t *ip) {
  if (ip == nullptr) {
    return false;
  }

  LinkerLocInfo info;
  const uint32_t *end_proc = nullptr;
  if (LinkerAPI::Instance().LocateAddress(const_cast<uint32_t*>(ip), info, false)) {
    end_proc = const_cast<const uint32_t*>(
        reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(info.addr) + info.size)));
  } else {
    EHLOG(INFO) << "ip must be java method written with assembly. pc : " << ip << maple::endl;
    return true;
  }

  // LSDA is placed just at end_proc, so end_proc is the start address of LSDA.
  if (*end_proc == kAbnormalFrameTag) {
    return true;
  }
  return false;
}

extern "C" void MRT_UpdateLastUnwindFrameLR(const uint32_t *lr) {
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  initialContext.frame.lr = lr;
  initialUnwindContext.SetEffective(true);
}

void UnwindContext::UpdateFrame(const uint32_t *pc, CallChain *fp) {
  frame.ip = pc;
  frame.fa = fp;
  frame.ra = nullptr;
  frame.lr = nullptr;
  interpFrame = nullptr;
}

extern "C" void MRT_UpdateLastJavaFrame(const uint32_t *pc, void *fa) {
  CallChain *fp = reinterpret_cast<CallChain*>(fa);
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  initialContext.UpdateFrame(pc, fp);
  initialUnwindContext.SetEffective(true);
}

extern "C" void MRT_UpdateLastUnwindContext(const uint32_t *pc, CallChain *fp, UnwindContextStatus status) {
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  initialContext.UpdateFrame(pc, fp);
  initialContext.status = status;
  if (status != UnwindContextStatusIsIgnored) {
    initialContext.SetInterpFrame(nullptr);
    initialContext.TagDirectJavaCallee(false);
  }
  initialUnwindContext.SetEffective(true);
}

extern "C" void MRT_UpdateLastUnwindContextIfReliable(const uint32_t *pc, void *fa) {
  CallChain *fp = reinterpret_cast<CallChain*>(fa);
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  if (initialContext.status == UnwindContextIsReliable) {
    initialContext.UpdateFrame(pc, fp);
  }
  initialUnwindContext.SetEffective(true);
}

extern "C" void MRT_SetRiskyUnwindContext(const uint32_t *pc, void *fa) {
  CallChain *fp = reinterpret_cast<CallChain*>(fa);
  MRT_UpdateLastUnwindContext(pc, fp, UnwindContextIsRisky);
}

extern "C" void MRT_SetReliableUnwindContextStatus() {
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  initialContext.status = UnwindContextIsReliable;
  initialContext.SetInterpFrame(nullptr);
  initialContext.TagDirectJavaCallee(false);
  initialUnwindContext.SetEffective(true);
}

#if !defined(__ANDROID__) || defined(__arm__)
extern "C" void MCC_SetRiskyUnwindContext(uint32_t *pc, void *fp)
__attribute__ ((alias ("MRT_SetRiskyUnwindContext")));

extern "C" void MCC_SetReliableUnwindContext()
__attribute__ ((alias ("MRT_SetReliableUnwindContextStatus")));
#endif // __ANDROID__

extern "C" void MRT_SetIgnoredUnwindContextStatus() {
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  initialContext.status = UnwindContextStatusIsIgnored;
  initialUnwindContext.SetEffective(true);
}

extern "C" void MCC_SetIgnoredUnwindContext()
__attribute__ ((alias ("MRT_SetIgnoredUnwindContextStatus")));

extern "C" void MRT_SaveLastUnwindContext(CallChain *frameAddr) {
  DCHECK(frameAddr != nullptr) << "frameAddr is nullptr in MRT_SaveLastUnwindContext" << maple::endl;
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();

  initialUnwindContext.SetEffective(false);
  UnwindData *unwindData = R2CFrame::GetUnwindData(frameAddr);
  unwindData->pc = initialContext.frame.ip;
  unwindData->fa = initialContext.frame.fa;
  unwindData->lr = initialContext.frame.lr;
  unwindData->ucstatus = static_cast<uintptr_t>(initialContext.status);
  unwindData->interpFrame = reinterpret_cast<uintptr_t>(initialContext.GetInterpFrame());
  unwindData->directCallJava = static_cast<uintptr_t>(initialContext.HasDirectJavaCallee());
  initialUnwindContext.SetEffective(true);
}

extern "C" void MRT_RestoreLastUnwindContext(CallChain *frameAddr) {
  InitialUnwindContext &initialUnwindContext = maplert::TLMutator().GetInitialUnwindContext();
  UnwindContext &initialContext = initialUnwindContext.GetContext();
  initialUnwindContext.SetEffective(false);
  initialContext.RestoreUnwindContextFromR2CStub(frameAddr);
  initialUnwindContext.SetEffective(true);
}

extern "C" void MRT_SetCurrentCompiledMethod(void *func) {
  maplert::TLMutator().SetCurrentCompiledMethod(func);
}
extern "C" void *MRT_GetCurrentCompiledMethod() {
  return maplert::TLMutator().GetCurrentCompiledMethod();
}

ATTR_NO_SANITIZE_ADDRESS
UnwindState BasicFrame::UnwindToMachineCaller(BasicFrame &caller) const {
  if (VLOG_IS_ON(eh)) {
    Check(this->HasFrameStructure(), ">> try to unwind an abnormal frame");
  }

  CallChain *curFp = this->fa; // backup current frame address
  CallChain *callerFp = curFp->callerFrameAddress;
  caller.fa = callerFp;
  caller.ip = this->ra;
  Check(caller.ip == curFp->returnAddress, "fatal: error callee frame for stack unwinding");

  if (caller.IsAnchorFrame()) {
    caller.ra = nullptr;
    return kUnwindFinish;
  } else {
    caller.ra = callerFp->returnAddress;
    return kUnwindSucc;
  }
}

void BasicFrame::Check(bool val, const char *msg) const {
  if (!val) {
    Dump(msg);
    MplDumpStack(msg);
    std::abort();
  }
}

// this method is able to obtain the nominal caller for java methods whose prologue and epilogue
// are eliminated during compilation.
ATTR_NO_SANITIZE_ADDRESS
UnwindState JavaFrame::UnwindToNominalCaller(JavaFrame &caller) const {
  if (VLOG_IS_ON(eh)) {
    Check(!this->IsAnchorFrame(), ">> try to unwind up from stack bottom frame");
  }

  if (this->HasFrameStructure()) {
    // in this case the parent frame is also the nominal caller.
    UnwindState state = BasicFrame::UnwindToMachineCaller(caller);
    caller.md = nullptr;
    caller.lr = nullptr;
    return state;
  } else {
    // in this case we construct a nominal caller.
    caller.fa = fa;
    caller.ip = ra;
    caller.lr = nullptr;
    caller.md = nullptr;

    if (caller.IsAnchorFrame()) {
      return kUnwindFinish;
    }

    CallChain *callerFp = caller.fa;
    DCHECK(callerFp != nullptr) << "caller frame address is null in JavaFrame::UnwindToNominalCaller" << maple::endl;
    caller.ra = callerFp->returnAddress;
    return kUnwindSucc;
  }
}

UnwindState JavaFrame::UnwindToJavaCallerFromRuntime(JavaFrame &frame, bool isEH) {
  size_t n = 0;
  while (n < MAPLE_STACK_UNWIND_STEP_MAX) {
    if (frame.IsAnchorFrame() || (isEH && frame.IsR2CFrame())) {
      return kUnwindFinish;
    }

    (void)frame.UnwindToNominalCaller(frame);

    if (frame.IsCompiledFrame()) {
      return kUnwindSucc;
    }
    ++n;
  }

  return kUnwindFail;
}

bool UnwindContext::IsStackBottomContext() const {
  if (interpFrame == nullptr) {
    return frame.IsAnchorFrame();
  } else {
    return false;
  }
}

ATTR_NO_SANITIZE_ADDRESS
void UnwindContext::RestoreUnwindContextFromR2CStub(CallChain *fa) {
  __MRT_ASSERT(fa != nullptr, "fatal error: frame pointer should be null");
  UnwindData *unwindData = R2CFrame::GetUnwindData(fa);
  status = static_cast<UnwindContextStatus>(unwindData->ucstatus);
  frame.ip = unwindData->pc;
  frame.fa = unwindData->fa;
  frame.lr = unwindData->lr;
  frame.ra = nullptr;
  interpFrame = reinterpret_cast<void*>(unwindData->interpFrame);

  CallChain *fp = frame.fa;
  if (fp != nullptr) {
    if (frame.lr != nullptr) {
      frame.ra = frame.lr;
    } else {
      frame.ra = fp->returnAddress;
    }
  }
}

UnwindState UnwindContext::UnwindToCallerContextFromR2CStub(UnwindContext& caller) const {
  __MRT_ASSERT(frame.IsR2CFrame(), "fatal error: current frame is not R2C stub");
  __MRT_ASSERT(status == UnwindContextIsReliable || status == UnwindContextIsRisky,
               "fatal error: R2C stub is not reliable for unwinding stack");

  UnwindData *unwindData = R2CFrame::GetUnwindData(frame.fa);
  UnwindContextStatus stubStatus = static_cast<UnwindContextStatus>(unwindData->ucstatus);

  switch (stubStatus) {
    case UnwindContextStatusIsIgnored: {
      caller.status = UnwindContextStatusIsIgnored;
      caller.interpFrame = reinterpret_cast<void*>(unwindData->interpFrame);
      __MRT_ASSERT(caller.interpFrame != nullptr, "fatal error: interp frame is null");
      caller.frame.Reset();
      break;
    }
    case UnwindContextIsReliable: {
      (void)frame.UnwindToNominalCaller(caller.frame);
      caller.status = UnwindContextIsReliable;
      caller.interpFrame = nullptr;
      break;
    }
    case UnwindContextIsRisky: {
      caller.RestoreUnwindContextFromR2CStub(frame.fa);
      caller.status = UnwindContextIsReliable;
      if (VLOG_IS_ON(eh)) {
        __MRT_ASSERT(caller.interpFrame == nullptr, "fatal error: interp frame is null");
      }
      break;
    }
    // Normally, there is only one case can go into here. A thread is executing MRT_SaveLastUnwindContext,
    // Suddenly receive a signal sent by another thread to unwind stack. This case is not guaranteed the status slot
    // of stub has been updated. So it may be an invalid status.
    default: {
      EHLOG(ERROR) << "Top function must be MRT_SaveLastUnwindContext, \
          Otherwise it is memory overwrite" << maple::endl;
      return kUnwindFinish;
    }
  }

  if (caller.IsStackBottomContext()) {
    return kUnwindFinish;
  }
  return kUnwindSucc;
}

UnwindState UnwindContext::UnwindToCallerContext(UnwindContext& caller, bool) const {
  // R2C stub is treated specially for unwinding stack because it saves unwind context transition.
  if (frame.IsR2CFrame()) {
    return UnwindToCallerContextFromR2CStub(caller);
  }

  switch (status) {
    case UnwindContextStatusIsIgnored: {
      // caller frame is obtained from *interpFrame* and must be an interpreted frame
      frame.Check(interpFrame != nullptr, "caller frame is expected to be an interpreted frame");
      return UnwindContextInterpEx::UnwindToCaller(interpFrame, caller);
    }
    case UnwindContextIsReliable: {
      caller.interpFrame = nullptr;
      caller.status = UnwindContextIsReliable;
      // caller frame is obtained by checking the structure of current context frame
      return frame.UnwindToNominalCaller(caller.frame);
    }
    case UnwindContextIsRisky: {
      if (VLOG_IS_ON(eh)) {
        // caller frame is obtained the value saved in UnwindContext.frame, and it is must be a compiled java frame
        frame.Check(frame.IsCompiledFrame(), "a compiled java frame is expected");
      }

      caller.frame = frame;
      caller.status = UnwindContextIsReliable;
      caller.interpFrame = nullptr;
      return kUnwindSucc;
    }

    default: {
      LOG(FATAL) << "UnwindToCallerContext status invalid" << maple::endl;
    }
  }
}

// obtain an unwind context as the start point of current thread.
UnwindState UnwindContext::GetStackTopUnwindContext(UnwindContext &context) {
  UnwindContext &lastContext = maplert::TLMutator().GetLastJavaContext();
  switch (lastContext.status) {
    case UnwindContextIsReliable: {
      lastContext.frame.Check(lastContext.interpFrame == nullptr, "");
      MRT_UNW_GETCALLERFRAME(context.frame);
      context.status = UnwindContextIsReliable;
      context.interpFrame = nullptr;
      break;
    }
    case UnwindContextIsRisky: {
      lastContext.frame.Check(lastContext.interpFrame == nullptr, "");
      context.status = UnwindContextIsReliable;
      context.frame = lastContext.frame;

      if (context.IsStackBottomContext()) {
        return kUnwindFinish;
      }
      // restore return address by frame structure since it is not maintained in this case
      if (context.frame.lr != nullptr) {
        context.frame.ra = context.frame.lr;
      } else {
        CallChain *curFp = lastContext.frame.fa;
        context.frame.ra = curFp->returnAddress;
      }
      break;
    }
    case UnwindContextStatusIsIgnored: {
      lastContext.frame.Check(lastContext.interpFrame != nullptr, "");
      context.status = UnwindContextStatusIsIgnored;
      context.interpFrame = lastContext.interpFrame;
      break;
    }

    default: std::abort();
  }
  return kUnwindFinish;
}

void JavaFrameInfo::ResolveFrameInfo() {
  if (resolved) {
    return;
  }

  const uint32_t *ip = javaFrame.GetFramePC();
  if (ip == nullptr) {
    return;
  }
  LinkerLocInfo info;
  if (LinkerAPI::Instance().LocateAddress(const_cast<uint32_t*>(ip), info, false)) {
    startIp = static_cast<uint32_t*>(info.addr);
    endIp = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(info.addr) + info.size);
    resolved = true;
  } else {
    EHLOG(INFO) << "Frame is java method written with assembly. pc : " << ip << maple::endl;
    startIp = nullptr;
    endIp = nullptr;
    return;
  }
  ResolveMethodMetadata();
}

void MapleStack::FastRecordCurrentJavaStack(std::vector<UnwindContext>& uwContextStack, size_t steps, bool resolveMD) {
  if (VLOG_IS_ON(eh)) {
    MplDumpStack("------------------ Fast Record Current Java Stack ------------------");
  }
  UnwindContext uwContext;
  (void) UnwindContext::GetStackTopUnwindContext(uwContext);

  size_t n = 0;
  while (n < steps && !uwContext.IsStackBottomContext()) {
    // Here you need to exclude the stack of the native implementation of the java method,
    // Otherwise the LocalAddress used in the later GetCallClass will trigger abort
    if (uwContext.IsCompiledContext()) {
      if (resolveMD) {
        uwContext.frame.ResolveMethodMetadata();
      }
      uwContextStack.push_back(uwContext);
      ++n;
    } else if (uwContext.IsInterpretedContext()) {
      uwContextStack.push_back(uwContext);
      ++n;
    }

    UnwindContext caller;
    if (UNLIKELY(uwContext.UnwindToCallerContext(caller) == kUnwindFinish)) {
      break;
    }
    uwContext = caller;
  }
}

void *MapleStack::VisitCurrentJavaStack(UnwindContext& uwContext,
                                        std::function<void *(UnwindContext&)> const filter) {
  if (VLOG_IS_ON(eh)) {
    MplDumpStack("------------------ Visit Current Java Stack ------------------");
  }

  (void) UnwindContext::GetStackTopUnwindContext(uwContext);

  size_t n = 0;
  void *retval = nullptr;
  while (n < MAPLE_STACK_UNWIND_STEP_MAX && !uwContext.IsStackBottomContext()) {
    if (uwContext.IsCompiledContext() ||
        uwContext.IsInterpretedContext()) {
      retval = filter(uwContext);
      if (retval != nullptr) {
        return retval;
      }
      ++n;
    }

    UnwindContext caller;
    if (UNLIKELY(uwContext.UnwindToCallerContext(caller) == kUnwindFinish)) {
      break;
    }
    uwContext = caller;
  }
  return retval;
}

void MapleStack::VisitJavaStackRoots(const UnwindContext &initialContext, const AddressVisitor &func, uint32_t tid) {
  UnwindContext context;
  UnwindState state = MapleStack::GetLastJavaContext(context, initialContext, tid);
  if (state != kUnwindSucc) {
    return;
  }

  while (!context.IsStackBottomContext()) {
    if (context.IsCompiledContext()) {
      // precise stack scan stack local var
      JavaFrameInfo frameInfo(context.frame);
      if (!frameInfo.IsAsmFrame()) {
        frameInfo.VisitGCRoots(func);
      }
    } else if (context.IsInterpretedContext()) {
      // visit gc roots in interp frame
      UnwindContextInterpEx::VisitGCRoot(context, func);
    }

    UnwindContext caller;
    if (UNLIKELY(context.UnwindToCallerContext(caller) == kUnwindFinish)) {
      break;
    }
    context = caller;
  }
  return;
}

static UnwindReasonCode UnwindBacktraceCallback(const _Unwind_Context *context, void *arg) {
  uintptr_t pc;
#if defined(__arm__)
  _Unwind_VRS_Get(const_cast<_Unwind_Context*>(context), _UVRSC_CORE, kPC, _UVRSD_UINT32, &pc);
#else
  pc = _Unwind_GetIP(const_cast<_Unwind_Context*>(context));
#endif
  std::vector<uint64_t> *stack = reinterpret_cast<std::vector<uint64_t>*>(arg);
  if (pc) {
    (*stack).push_back(pc);
  } else {
    LOG(INFO) << "UnwindBacktrace is failed to get pc" << maple::endl;
  }
  return _URC_NO_REASON;
}

void MapleStack::FastRecordCurrentStackPCsByUnwind(std::vector<uint64_t> &callStack, size_t step) {
  (void)_Unwind_Backtrace(UnwindBacktraceCallback, &callStack);
  if (callStack.size() > step) {
    callStack.erase(callStack.begin() + step, callStack.end());
  }
}

struct sigaction act;
struct sigaction oldact;

// The LOG interface cannot be used in this function to prevent deadlocks.
static void RemoteUnwindHandler(int, siginfo_t*, void *ucontext) {
  sigaction(SIGRTMIN, &oldact, nullptr);
  Mutator *mutator = CurrentMutatorPtr();
  if (mutator == nullptr) {
    LOG(FATAL) << "current mutator should not be nullptr " << ucontext << maple::endl;
  }

#if defined(__aarch64__)
  mcontext_t *mcontext = &(reinterpret_cast<ucontext_t*>(ucontext)->uc_mcontext);
  if (!mutator->InSaferegion()) {
    if (LinkerAPI::Instance().IsJavaText(reinterpret_cast<void*>(mcontext->pc))) {
      InitialUnwindContext &initialUnwindContext = mutator->GetInitialUnwindContext();
      UnwindContext &initialContext = initialUnwindContext.GetContext();
      initialContext.frame.fa = reinterpret_cast<CallChain*>(mcontext->regs[Register::kFP]);
      initialContext.frame.ip = reinterpret_cast<uint32_t*>(mcontext->pc + kAarch64InsnSize);
    } else {
      while (!remoteUnwinder.IsUnwinderIdle()) {}
      remoteUnwinder.SetRemoteIdle(true);
      return;
    }
  }
#elif defined(__arm__)
  if (!mutator->InSaferegion()) {
    while (!remoteUnwinder.IsUnwinderIdle()) {}
    remoteUnwinder.SetRemoteIdle(true);
    return;
  }
#endif

  // Last java frame update is finished,wake up the request thread to unwind.
  remoteUnwinder.SetRemoteUnwindStatus(kRemoteLastJavaFrameUpdataFinish);

  // Waiting for request thread to finish unwind
  if (!remoteUnwinder.WaitRemoteUnwindStatus(kRemoteUnwindFinish)) {
    while (!remoteUnwinder.IsUnwinderIdle()) {}
    remoteUnwinder.SetRemoteIdle(true);
    return;
  }

  remoteUnwinder.SetRemoteUnwindStatus(kRemoteTargetThreadSignalHandlingExited);
  while (!remoteUnwinder.IsUnwinderIdle()) {}
  remoteUnwinder.SetRemoteIdle(true);
}

void CheckMutatorListLock(uint32_t tid) {
  uint64_t times = 0;
  constexpr uint64_t sleepTime = 5000;   // 5us
  constexpr uint64_t maxTimes = 1000000; // 1sec
  while (!MutatorList::Instance().TryLock()) {
    ++times;
    if (times > maxTimes) {
      LOG(FATAL) << "Get Mutator Timeout : tid = " << tid << maple::endl;
    }
    timeutils::SleepForNano(sleepTime);
  }
}

void RecordStackFromUnwindContext(std::vector<uint64_t> &callStack, size_t steps, UnwindContext &context) {
  size_t n = 0;
  while (n < steps && !context.IsStackBottomContext()) {
    // A program counter (PC) is a CPU register in the computer processor which has the address of the next
    // instruction to be executed from memory. so the address of current instruction should minus 4.
    // Here you need to exclude the stack of the native implementation of the java method,
    // Otherwise the LocalAddress used in the later GetCallClass will trigger abort
    if (!UnwindContextInterpEx::TryRecordStackFromUnwindContext(callStack, context)) {
      // The lowest ip bit in r2c frame is marked with 1 and needs to be filtered out and dec 1.
      if (reinterpret_cast<uint64_t>(context.frame.ip) & kDexMethodTag) {
        callStack.push_back(reinterpret_cast<uint64_t>(context.frame.ip) - kDexMethodTag - kAarch64InsnSize);
      } else {
        callStack.push_back(reinterpret_cast<uint64_t>(context.frame.ip) - kAarch64InsnSize);
      }
    }

    ++n;
    UnwindContext caller;
    if (UNLIKELY(context.UnwindToCallerContext(caller) == kUnwindFinish)) {
      break;
    }
    context = caller;
  }
}

// At present, when printing call stack in maple, Java method implemented by native is not
// included, because this kind of method is compiled by g++ compiler without metedata,
// so it can not be printed when printing call stack.
void MapleStack::FastRecordCurrentStackPCs(std::vector<uint64_t> &callStack, size_t steps) {
  if (VLOG_IS_ON(eh)) {
    MplDumpStack("------------------ Fast Record Current Java Stack PCs ------------------");
  }

  UnwindContext context;
  (void)UnwindContext::GetStackTopUnwindContext(context);

  RecordStackFromUnwindContext(callStack, steps, context);
}

void MapleStack::RecordStackPCs(std::vector<uint64_t> &callStack, uint32_t tid, size_t steps) {
  std::lock_guard<std::mutex> lock(remoteUnwinder.GetRemoteUnwindLock());
  // Reserve memory space for the vector. When you push back, the memory will not be
  // applied. Avoid deadlock with target thread
  callStack.reserve(steps);

  if (tid == static_cast<uint32_t>(maple::GetTid())) {
    LOG(ERROR) << "The target thread is the same as the current thread" << maple::endl;
    remoteUnwinder.SetUnwinderIdle(true);
    return;
  }

  CheckMutatorListLock(tid);

  Mutator *mutator = MutatorList::Instance().GetMutator(tid);
  if (mutator == nullptr) {
    MutatorList::Instance().Unlock();
    return;
  }

  if (!remoteUnwinder.WaitForRemoteIdle()) {
    MutatorList::Instance().Unlock();
    return;
  }

  remoteUnwinder.SetUnwinderIdle(false);
  remoteUnwinder.SetRemoteIdle(false);
  remoteUnwinder.SetRemoteUnwindStatus(kRemoteUnwindIdle);

  // register signal hander function
  (void)memset_s(&act, sizeof(act), 0, sizeof(act));
  (void)memset_s(&oldact, sizeof(oldact), 0, sizeof(oldact));
  act.sa_sigaction = RemoteUnwindHandler;
  act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGRTMIN, &act, &oldact) != 0) {
    LOG(ERROR) << "sigaction failed" << maple::endl;
    return;
  }

  LOG(INFO) << "send signal to tid " << tid << maple::endl;

  // ***************************************************************************
  // From now on until the SignalHandler exits, you can't use the LOG interface,
  // because if the target thread is in the LOG state, a deadlock will occur.
  // ***************************************************************************
  if (maple::TgKill(static_cast<uint32_t>(maple::GetPid()), tid, SIGRTMIN) != 0) {
    MutatorList::Instance().Unlock();
    remoteUnwinder.SetRemoteIdle(true);
    remoteUnwinder.SetUnwinderIdle(true);
    sigaction(SIGRTMIN, &oldact, nullptr);
    return;
  }

  // waiting for the target thread to complete the last java frame update
  if (!remoteUnwinder.WaitRemoteUnwindStatus(kRemoteLastJavaFrameUpdataFinish)) {
    MutatorList::Instance().Unlock();
    remoteUnwinder.SetUnwinderIdle(true);
    return;
  }

  UnwindContext uwContext;
  InitialUnwindContext &initialUnwindContext = mutator->GetInitialUnwindContext();

  // At this time, the target thread has entered the signal handling. The validity of the
  // corresponding mutator can be guaranteed and so the lock can be released.
  MutatorList::Instance().Unlock();
  UnwindState state = kUnwindFail;
  if (initialUnwindContext.IsEffective()) {
    const UnwindContext &initialContext = initialUnwindContext.GetContext();
    state = GetLastJavaContext(uwContext, initialContext, tid);
  }

  if (state == kUnwindFail) {
    // After the kill signal is sent, it must be exited by the handshake
    // mechanism, otherwise the corresponding thread will always wait.
    remoteUnwinder.SetRemoteUnwindStatus(kRemoteUnwindFinish);
    if (!remoteUnwinder.WaitRemoteUnwindStatus(kRemoteTargetThreadSignalHandlingExited)) {
      remoteUnwinder.SetUnwinderIdle(true);
      return;
    }
    remoteUnwinder.SetUnwinderIdle(true);
    return;
  }

  RecordStackFromUnwindContext(callStack, steps, uwContext);

  // After the kill signal is sent, it must be exited by the handshake
  // mechanism, otherwise the corresponding thread will always wait.
  remoteUnwinder.SetRemoteUnwindStatus(kRemoteUnwindFinish);

  if (!remoteUnwinder.WaitRemoteUnwindStatus(kRemoteTargetThreadSignalHandlingExited)) {
    remoteUnwinder.SetUnwinderIdle(true);
    return;
  }
  remoteUnwinder.SetUnwinderIdle(true);
}

UnwindState GetNextJavaFrame(UnwindContext &context) {
  UnwindState state = kUnwindFail;
  while (!context.IsStackBottomContext()) {
    if (context.IsCompiledContext()) {
      context.interpFrame = nullptr;
      context.status = UnwindContextIsReliable;
      state = kUnwindSucc;
      break;
    }

    if (context.IsInterpretedContext()) {
      context.status = UnwindContextStatusIsIgnored;
      state = kUnwindSucc;
      break;
    }

    UnwindContext caller;
    if (UNLIKELY(context.UnwindToCallerContext(caller) == kUnwindFinish)) {
      state = kUnwindFinish;
      break;
    }
    context = caller;
  }
  return state;
}

// NEED: support enable USE_ZTERP in phone. Similar with other functions, need refactor.
UnwindState MapleStack::GetLastJavaContext(UnwindContext &context, const UnwindContext &initialContext, uint32_t tid) {
  if (VLOG_IS_ON(eh)) {
    EHLOG(INFO) << "GetLastJavaContext tid : " << tid << maple::endl;
  }
  if (initialContext.IsStackBottomContext()) {
    return kUnwindFinish;
  }

  UnwindState state = kUnwindFail;

  switch (initialContext.status) {
    case UnwindContextStatusIsIgnored: {
      initialContext.frame.Check(initialContext.interpFrame != nullptr, "");
      context.status = UnwindContextStatusIsIgnored;
      context.interpFrame = initialContext.interpFrame;
      context.frame.Reset();
      state = kUnwindSucc;
      break;
    }
    case UnwindContextIsReliable:
    case UnwindContextIsRisky: {
      JavaFrame frame = initialContext.frame;
      CallChain *curFp = frame.fa;
      if (VLOG_IS_ON(eh)) {
        MplCheck(curFp != 0, "latest java frame address should not be null");
      }
      if (frame.lr) {
        // this frame does not have frame structure.
        frame.ra = frame.lr;
      } else {
        // this frame does have frame structure so we obtain return address as ABI described.
        frame.ra = curFp->returnAddress;
      }

      frame.SetMetadata(nullptr);
      context.frame = frame;
      context.status = initialContext.status;
      state = GetNextJavaFrame(context);
      break;
    }
    default: std::abort();
  }
  return state;
}


// If the parameter isEH is true, it means that the function may recognize n2j frame,
// which needs to be set to true when calling the function in exception handling, and
// false when stacking.
UnwindState MapleStack::GetLastJavaFrame(JavaFrame &frame, const UnwindContext *initialContext, bool isEH) {
  bool unwindAnotherThread = true;
  if (initialContext == nullptr) {
    initialContext = &(TLMutator().GetLastJavaContext());
    unwindAnotherThread = false;
  }

  UnwindContextStatus status = initialContext->status;

  UnwindState state = kUnwindFail;
  if (status == UnwindContextIsReliable && !unwindAnotherThread) {
    MRT_UNW_GETCALLERFRAME(frame);
    state = frame.UnwindToJavaCallerFromRuntime(frame, isEH);
  } else {
    frame = initialContext->frame;

    // For rare cases (like in reference collector thread), *initialContext* may holds an anchor frame.
    if (frame.IsAnchorFrame()) {
      state = kUnwindFinish;
      return state;
    } else {
      CallChain *curFp = frame.fa;
      MplCheck(curFp != 0, "latest java frame address should not be null");

      if (frame.lr) {
        // this frame does not have frame structure.
        frame.ra = frame.lr;
      } else {
        // this frame does have frame structure so we obtain return address as ABI described.
        frame.ra = curFp->returnAddress;
      }

      frame.SetMetadata(nullptr);
    }

    if (frame.IsCompiledFrame()) {
      state = kUnwindSucc;
    } else {
      state = frame.UnwindToJavaCallerFromRuntime(frame);
    }
  }

  if (VLOG_IS_ON(eh)) {
    std::stringstream ss;
    frame.Dump(">>>> Get stack-top java frame ", ss);
    EHLOG(INFO) << ss.str() << maple::endl <<
        "\t" << "frame context is " <<
        (status == UnwindContextIsReliable ? "Reliable" : "Risky") <<
        " -- Unwind " << (state == kUnwindSucc ? "Succeeded" : "Finished") << maple::endl;
  }

  return state;
}

#ifdef MAPLE_RECORD_METHOD_INFO_AT_START_PROC
extern "C" MRT_EXPORT bool MapleGetJavaMethodNameByIP(const uint32_t *ip, std::string &funcName, uint64_t *funcOffset) {
  LinkerLocInfo info;
  bool isJava = LinkerAPI::Instance().LocateAddress(ip, info, false);
  if (isJava) {
    if (VLOG_IS_ON(eh)) {
      EHLOG(INFO) << std::hex << "obtain java method name for pc " << reinterpret_cast<uintptr_t>(ip) << " (" <<
          info.addr << " ~ " << reinterpret_cast<uintptr_t>(info.addr) + info.size << ")" <<
          std::dec << maple::endl;
    }

    uint64_t *md = static_cast<uint64_t*>(
        JavaFrame::GetMethodMetadata(reinterpret_cast<uint32_t*>(info.addr)));
    if (UNLIKELY(!JavaFrame::GetMapleMethodFullName(funcName, md, ip))) {
      EHLOG(ERROR) << "JavaFrame::GetMapleMethodFullName() in MapleGetJavaMethodNameByIP() return false." <<
          maple::endl;
    }
    if (funcOffset == nullptr) {
      EHLOG(FATAL) << "funcOffset is nullptr" << maple::endl;
    }
    *funcOffset = reinterpret_cast<uint64_t>(ip) - reinterpret_cast<uint64_t>(info.addr);
    return funcName.size() > 0;
  }
  return false;
}
#endif // MAPLE_RECORD_METHOD_INFO_AT_START_PROC
} // namespace maplert
