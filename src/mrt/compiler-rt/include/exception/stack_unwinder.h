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

// Stack unwinder
//
// The stack unwinder iterates through the frames of a stack from the top to
// the bottom.  The main purpose is:
//
// 1. During garbage collection, it reveals the register and stack states at the
// call site of each frame (without modifying the frames) in order to helps the
// stack scanner identify object references held on the stack.
//
// It may also server the following purposes:
//
// 2. During exception handling, it removes stack frames from the top of the
// stack.  Alternatively, exception handling can be done using the system ABI.
//
// The design is mainly inspired by:
//
// a) libunwind
//    Original implementation: http://www.nongnu.org/libunwind/
//    LLVM implementation: http://releases.llvm.org/download.html
// b) The stack introspection API for the Mu micro virtual machine
//    https://gitlab.anu.edu.au/mu/mu-spec/blob/master/api.rst#stack-introspection
//
#ifndef __MAPLERT_STACKUNWINDER_H__
#define __MAPLERT_STACKUNWINDER_H__

#include <cstdint>
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <jni.h>
#include <utils/time_utils.h>

#include "cinterface.h"
#include "linker_api.h"
#include "eh_personality.h"
#include "metadata_layout.h"

#define MAPLE_STACK_UNWIND_STEP_MAX 8000UL

namespace maplert {
enum UnwindState {
  kUnwindFail   = 0,
  kUnwindSucc   = 1,
  kUnwindFinish = 2,
};

// this data struct matches the machine frame layout for both aarch64 and arm.
//                 |  ...           |
//                 |  ...           |
//                 |  lr to caller  | return address for this frame
//        fp  -->  |  caller fp     |
//                 |  ...           |
//        sp  -->  |  ...           |
// pointer to CallChain is actually frame address (fp)
struct CallChain {
  CallChain *callerFrameAddress;
  const uint32_t *returnAddress;
};

// do not ever define any virtual method for BasicFrame
// we should rename BasicFrame to MachineFrame
class BasicFrame {
 public:
  // fa: frame address of this frame, for now this is frame pointer
  CallChain *fa;

  // ip: instruction pointer, the value in PC register when unwinding this frame,
  // which points to the instruction when control flow returns from callee to this frame.
  const uint32_t *ip;

  // ra: return address of this frame. For a normal frame, this can
  // be retrieved from frame address,
  const uint32_t *ra;

  // lr: Specific to frames which do not have a normal frame structure.
  // This field is 0 for a normal frame, because we can always retrieve return address from CallChain.
  // If not null, this field indicates the return address.
  // This is necessary for segv occured in a frame without data on stack .
  const uint32_t *lr;

  void Reset() {
    ip = nullptr;
    fa = nullptr;
    lr = nullptr;
    ra = nullptr;
  }

  BasicFrame() {
    Reset();
  }

  BasicFrame(const uint32_t *ip, CallChain *fa, const uint32_t *ra) : fa(fa), ip(ip), ra(ra), lr(nullptr) {}

  explicit BasicFrame(const BasicFrame &frame) {
    CopyFrom(frame);
  }

  BasicFrame& operator=(const BasicFrame &frame) {
    CopyFrom(frame);
    return *this;
  }

  virtual ~BasicFrame() = default;

  void Dump(const std::string msg, std::stringstream &ss) const {
    static constexpr size_t defaultBufSize = 128;
    char buf[defaultBufSize];
    if (snprintf_s(buf, sizeof buf, sizeof buf,
                   ": frame pc %p, frame address %p, retrun to %p, link register %p", ip, fa, ra, lr) < 0) {
      LOG(ERROR) << "Dump snprintf_s fail" << maple::endl;
    }
    ss << msg << buf;
  }

  void Dump(const std::string msg) const {
    std::stringstream ss;
    Dump(msg, ss);
    EHLOG(INFO) << ss.str() << maple::endl;
  }

  void Check(bool val, const char *msg) const;

  const uint32_t *GetFramePC() const {
    return ip;
  }

  void SetFramePC(const void *pc) {
    ip = static_cast<const uint32_t*>(pc);
  }

  CallChain *GetFrameAddress() const {
    return fa;
  }

  const uint32_t *GetReturnAddress() const {
    return ra;
  }

  const uint32_t *GetLR() const {
    return lr;
  }

  inline bool IsCompiledFrame() const {
    return LinkerAPI::Instance().IsJavaText(ip);
  }

  inline bool IsR2CFrame() const {
    return (reinterpret_cast<uintptr_t>(ra) & 1) == 1;
  }

  // note: this method takes advantage of JavaMethodHasNoFrame
  inline bool HasFrameStructure() const {
    return lr == nullptr;
  }

  // The anchor frame is conceptually a sentinel frame, whose
  // fp/ip is all zeros. This frame does not exists but is natually a good
  // choice to detect the completion of unwinding if we initialize fp/ip to null.
  inline bool IsAnchorFrame() const {
    return (fa == nullptr);
  }

  inline void CopyFrom(const BasicFrame &frame) {
    ip = frame.ip;
    fa = frame.fa;
    ra = frame.ra;
    lr = frame.lr;
  }

  // return kUnwindSucc or kUnwindFinish only, do not return kUnwindFail.
  // caller assures this frame is a normal frame.
  // we name the direct caller frame in machine stack with "machine caller".
  UnwindState UnwindToMachineCaller(BasicFrame &caller) const;
};

// JavaFrame should be renamed to CommonFrame or just Frame
class JavaFrame : public BasicFrame {
 public:
  JavaFrame() : BasicFrame() {
    md = nullptr;
  }

  // caller assures this frame is a normal frame.
  UnwindState UnwindToNominalCaller(JavaFrame &caller) const;
  UnwindState UnwindToJavaCallerFromRuntime(JavaFrame &frame, bool isEH = false);

  // format: class|method|signature
  MRT_EXPORT static bool GetJavaMethodFullName(std::string &name, const uint64_t *md);

  // method|signature
  MRT_EXPORT static void GetJavaMethodSignatureName(std::string &name, const void *md);
  // method
  MRT_EXPORT static void GetJavaMethodName(std::string &name, const void *md);
  // class: A.B.C
  MRT_EXPORT static void GetJavaClassName(std::string &name, const uint64_t *md);

  MRT_EXPORT static jclass GetDeclaringClass(const uint64_t *md);

  MRT_EXPORT static uint64_t *GetMethodMetadata(const uint32_t *startPC);
  MRT_EXPORT static const MethodDesc *GetMethodDesc(const uint32_t *startPC);

  MRT_EXPORT static uint64_t GetRelativePc(
      const std::string &soName, uint64_t pc, LinkerMFileInfo *mplInfo = nullptr);

  static bool GetMapleMethodFullName(std::string &name, const uint64_t *md, const void *ip);
  // format: <class-name>|<method-name>|<signature>
  MRT_EXPORT static bool GetMapleMethodFullName(std::string &name, const uint64_t *md);

  // Maple-mangled format
  static void GetMapleClassName(std::string &name, const uint64_t *md);

  // for resolving symbols of call stack
  static bool DumpProcInfo(uintptr_t pc, std::string *name = nullptr);
  static void DumpProcInfoLog(jlong elem);

  static bool JavaMethodHasNoFrame(const uint32_t *ip);

  inline void ResolveMethodMetadata() {
    if (md == nullptr) {
      LinkerLocInfo info;
      if (LinkerAPI::Instance().LocateAddress(const_cast<uint32_t*>(ip), info, false)) {
        md = static_cast<uint64_t*>(
            GetMethodMetadata(reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(info.addr))));
      }
    }
  }

  inline jclass GetDeclaringClass() {
    ResolveMethodMetadata();
    return GetDeclaringClass(md);
  }

  // format: class|method|signature
  inline void GetJavaMethodFullName(std::string &name) {
    ResolveMethodMetadata();
    (void)GetJavaMethodFullName(name, md);
  }
  // method|signature
  inline void GetJavaMethodSignatureName(std::string &name) {
    ResolveMethodMetadata();
    GetJavaMethodSignatureName(name, md);
  }

  // method
  inline void GetJavaMethodName(std::string &name) {
    ResolveMethodMetadata();
    GetJavaMethodName(name, md);
  }

  // class: A.B.C
  inline void GetJavaClassName(std::string &name) {
    ResolveMethodMetadata();
    GetJavaClassName(name, md);
  }

  // Maple-mangled format
  inline void GetMapleClassName(std::string &name) {
    ResolveMethodMetadata();
    GetMapleClassName(name, md);
  }

  inline const uint64_t *GetMetadata() const {
    return md;
  }

  inline void SetMetadata(const uint64_t *metadata) {
    md = metadata;
  }
 private:
  const uint64_t *md; // mMD: Metadata for this Java method, nullptr indicates unresolved
};

// UnwindData is a helper for retrieving previous java context.
// this data struct should match the layout defined in r2c_stub
struct UnwindData {
  const uint32_t *pc;
  CallChain *fa;
  const uint32_t *lr;
  uintptr_t ucstatus;
  uintptr_t interpFrame;
  uintptr_t directCallJava;
};

// R2CFrame is a stub frame constructed by runtime code to invoke compiled java methods. R2CFrame saves
// its java caller context as it is for unwinding. R2CStub is implemented in r2c_stub_xx.S.
// Runtime code must be always compiled with -fno-omit-frame-pointer to maintain standard frame layout according to ABI.
class R2CFrame : public BasicFrame {
 public:
  R2CFrame() : BasicFrame() {}
  R2CFrame(const uint32_t *ip, CallChain *fa, const uint32_t *ra) : BasicFrame(ip, fa, ra) {}

  // return kUnwindSucc or kUnwindFinish only, do not return kUnwindFail
  UnwindState UnwindToJavaCaller(JavaFrame &caller) const;

  static UnwindData *GetUnwindData(CallChain *fa) {
    return reinterpret_cast<UnwindData*>(fa + kOffsetForUnwindData);
  }

  UnwindData *GetUnwindData() {
    return GetUnwindData(this->fa);
  }
 private:
  static constexpr size_t kOffsetForUnwindData = 1;
};

enum UnwindContextStatus {
  UnwindContextStatusIsIgnored = 0x12345678, // ignore this status when unwinding to caller context
  UnwindContextIsReliable = 1, // we should unwind these frames one by one
  UnwindContextIsRisky    = 2, // we should cross these frames to get to previous java frame
};

extern "C" void MRT_UpdateLastUnwindFrameLR(const uint32_t *lr);
extern "C" void MRT_UpdateLastUnwindContext(const uint32_t *pc, CallChain *fp, UnwindContextStatus status);

// UnwindContext contains all information needed for stack unwinding.
// *frame* is current frame which is start point for stack unwinding.
// If *status* is UnwindContextIsReliable, caller frame is constructed by inspecting frame structure according to ABI.
// Otherwise, if *status* is UnwindContextIsRisky, caller frame is constructed either from TLS when we get
// last-java-frame, or by inspecting R2J frame structure if *frame* is a R2J frame.
struct UnwindContext {
  UnwindContextStatus status = UnwindContextIsRisky;
  JavaFrame frame; // BasicFrame
  void *interpFrame = nullptr; // if not null, some interpreted frame is the last java frame

  bool hasDirectJavaCallee = false; // tag when interpreted method calls a compiled method, used only by R2C stub
 public:
  inline void *GetInterpFrame() {
    return interpFrame;
  }
  inline void SetInterpFrame(void *newInterpFrame) {
    interpFrame = newInterpFrame;
  }
  inline bool HasDirectJavaCallee() const {
    return hasDirectJavaCallee;
  }
  inline void TagDirectJavaCallee(bool val) {
    hasDirectJavaCallee = val;
  }
  inline void Reset() {
    status = UnwindContextIsRisky;
    frame.Reset();
    interpFrame = nullptr;
    hasDirectJavaCallee = false;
  }
  inline void CopyFrom(const UnwindContext &context) {
    this->status = context.status;
    this->interpFrame = context.interpFrame;
    this->hasDirectJavaCallee = context.hasDirectJavaCallee;
    this->frame.CopyFrom(context.frame);
  }

  bool IsCompiledContext() const {
    return frame.IsCompiledFrame();
  }
  bool IsInterpretedContext() const {
    return (interpFrame != nullptr);
  }
  bool IsJavaContext() const {
    return IsCompiledContext() || IsInterpretedContext();
  }
  bool IsStackBottomContext() const;

  UnwindState UnwindToCallerContext(UnwindContext &caller, bool isEH = false) const;
  UnwindState UnwindToCallerContextFromR2CStub(UnwindContext &caller) const;
  void UpdateFrame(const uint32_t *pc, CallChain *fp);

  void RestoreUnwindContextFromR2CStub(CallChain *fa); // fa is the address of R2CStub frame
  static UnwindState GetStackTopUnwindContext(UnwindContext &context);
};

struct InitialUnwindContext {
 public:
  UnwindContext &GetContext() {
    return uwContext;
  }

  bool IsEffective() const {
    return effective.load(std::memory_order_acquire);
  }

  void SetEffective(bool value) {
    effective.store(value, std::memory_order_release);
  }

  void CopyFrom(InitialUnwindContext &context) {
    uwContext.CopyFrom(context.GetContext());
    effective.store(context.effective.load(std::memory_order_acquire), std::memory_order_release);
  }

 private:
  UnwindContext uwContext;
  std::atomic<bool> effective = { true };
};

class JavaFrameInfo {
 public:
  void VisitGCRoots(const AddressVisitor &func) {
    const JavaFrame &thisFrame = GetJavaFrame();
    const MethodDesc *desc = JavaFrame::GetMethodDesc(GetStartPC());
    uintptr_t localRefStart = reinterpret_cast<uintptr_t>(thisFrame.GetFrameAddress()) + desc->localRefOffset;
    for (size_t i = 0; i < desc->localRefNumber; ++i) {
      uintptr_t localRefSlot = localRefStart + i * sizeof(address_t);
      address_t &localRef = *reinterpret_cast<address_t*>(localRefSlot);
      func(localRef);
    }
  }

  const uint32_t *GetEndPC() const {
    return endIp;
  }

  void SetEndPC(uint32_t *pc) {
    endIp = pc;
  }

  const uint32_t *GetStartPC() const {
    return startIp;
  }

  void SetStartPC(uint32_t *pc) {
    startIp = pc;
  }

  // These asm frame are in the jave section, but not in any java type.
  bool IsAsmFrame() const {
    return (startIp == nullptr);
  }

  const JavaFrame &GetJavaFrame() const {
    return javaFrame;
  }

  inline void Build(const JavaFrame &frame) {
    this->javaFrame = frame;
    ResolveFrameInfo();
  }

  inline void CopyFrom(const JavaFrameInfo &frameInfo) {
    this->javaFrame = frameInfo.javaFrame;
    this->startIp = frameInfo.startIp;
    this->endIp = frameInfo.endIp;
    this->resolved = frameInfo.resolved;
  }

  JavaFrameInfo(uint32_t *startIP, uint32_t *endIP, const void *pc) {
    startIp = startIP;
    endIp = endIP;
    javaFrame.SetFramePC(pc);
  }

  explicit JavaFrameInfo(const JavaFrame &frame) {
    Build(frame);
  }

  explicit JavaFrameInfo(const JavaFrameInfo &frameInfo) {
    CopyFrom(frameInfo);
  };

  virtual ~JavaFrameInfo() = default;
 protected:
  JavaFrame javaFrame;
  uint32_t *startIp;
  uint32_t *endIp;       // endIp == lsda, when the lsda is next to the .cfi_endproc
  bool resolved = false; // true if proc_info_ is valid

  inline void ResolveMethodMetadata() {
    if (javaFrame.GetMetadata() == nullptr && startIp != nullptr) {
      uint64_t *md = JavaFrame::GetMethodMetadata(reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(startIp)));
      javaFrame.SetMetadata(md);
    }
  }

  void ResolveFrameInfo();

  virtual void Dump(const std::string msg, std::stringstream &ss) {
    std::string name;
    javaFrame.GetJavaMethodFullName(name);
    ss << msg << " --- java frame info " << "\n" <<
        "\t" << "name         : " << name << "\n" << std::hex <<
        "\t" << "current pc   : " << javaFrame.ip << "\n" <<
        "\t" << "start pc     : " << startIp << "\n" <<
        "\t" << "end pc       : " << endIp << "\n" <<
        std::dec;
  }
};

using RemoteUnwindStatus = enum {
  kRemoteUnwindIdle,
  kRemoteLastJavaFrameUpdataFinish,
  kRemoteUnwindFinish,
  kRemoteTargetThreadSignalHandlingExited,
};

class RemoteUnwinder {
 public:
  RemoteUnwinder() {
    remoteUnwindStatus = kRemoteUnwindIdle;
    remoteIdle = true;
    unwinderIdle = true;
  }

  ~RemoteUnwinder() = default;

  std::mutex &GetRemoteUnwindLock() {
    return remoteUnwindLock;
  }

  void SetRemoteUnwindStatus(RemoteUnwindStatus status) {
    remoteUnwindStatus = status;
  }

  void SetRemoteIdle(bool status) {
    remoteIdle = status;
  }

  void SetUnwinderIdle(bool status) {
    unwinderIdle = status;
  }

  bool IsUnwinderIdle() const {
    return unwinderIdle;
  }

  bool WaitForRemoteIdle() const {
    uint64_t times = 0;
    constexpr uint64_t maxTimes = 500000;
    while (remoteIdle != true) {
      ++times;
      if (times > maxTimes) {
        return false;
      }
    }
    return true;
  }

  bool WaitRemoteUnwindStatus(RemoteUnwindStatus status) const {
    uint64_t times = 0;
    constexpr uint64_t maxTimes = 500000;
    if (remoteIdle == true) {
      return false;
    }
    while (remoteUnwindStatus != status) {
      ++times;
      if (times > maxTimes) {
        return false;
      }
    }
    return true;
  }

 private:
  std::atomic<bool> remoteIdle;
  std::atomic<bool> unwinderIdle;
  std::mutex remoteUnwindLock;
  std::atomic<RemoteUnwindStatus> remoteUnwindStatus;
};

class MapleStack {
 public:
  MapleStack() = default;
  ~MapleStack() = default;

  // the last java frame is on the top of java call stack.
  static UnwindState GetLastJavaContext(UnwindContext &context, const UnwindContext &initialContext, uint32_t tid);
  static UnwindState GetLastJavaFrame(JavaFrame &frame, const UnwindContext *initialContext = nullptr,
                                      bool isEH = false);


  const inline std::vector<JavaFrame> &GetStackFrames() {
    return stackFrames;
  }
  inline std::vector<UnwindContext> &GetContextStack() {
    return uwContextStack;
  }

  inline void AddFrame(const JavaFrame &frame) {
    stackFrames.push_back(frame);
  }

  // visit GC roots of current java thread for tracing GC
  static void VisitJavaStackRoots(const UnwindContext &initialContext, const AddressVisitor &func, uint32_t tid);

  MRT_EXPORT static void FastRecordCurrentJavaStack(std::vector<UnwindContext> &uwContextStack,
                                                    size_t steps = MAPLE_STACK_UNWIND_STEP_MAX,
                                                    bool resolveMD = false);

  MRT_EXPORT static void FastRecordCurrentJavaStack(MapleStack &callStack,
                                                    size_t steps = MAPLE_STACK_UNWIND_STEP_MAX,
                                                    bool resolveMD = false) {
    FastRecordCurrentJavaStack(callStack.GetContextStack(), steps, resolveMD);
  }

  MRT_EXPORT static void *VisitCurrentJavaStack(UnwindContext &uwContext,
                                                std::function<void* (UnwindContext&)> filter);

  MRT_EXPORT static void FastRecordCurrentStackPCsByUnwind(std::vector<uint64_t> &framePCs,
                                                           size_t steps = MAPLE_STACK_UNWIND_STEP_MAX);

  MRT_EXPORT static void FastRecordCurrentStackPCs(std::vector<uint64_t> &framePCs,
                                                   size_t steps = MAPLE_STACK_UNWIND_STEP_MAX);

  MRT_EXPORT static void RecordStackPCs(std::vector<uint64_t> &framePCs, uint32_t tid,
                                        size_t steps = MAPLE_STACK_UNWIND_STEP_MAX);

 private:
  std::vector<JavaFrame> stackFrames;
  std::vector<UnwindContext> uwContextStack;
};
} // namespace maplert

#endif // __MAPLERT_STACKUNWINDER_H__
