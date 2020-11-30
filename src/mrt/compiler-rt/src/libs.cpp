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
#include "libs.h"
#include <algorithm>
#include <dlfcn.h>
#include "exception/exception_handling.h"
#include "tracer.h"
#include "mm_config.h"
#include "mrt_libs_api.h"
#include "mrt_monitor_api.h"
#include "exception/mrt_exception.h"
#include "mrt_handleutil.h"
#include "mrt_methodhandle_mpl.h"
#include "profile.h"
#include "object_base.h"
#include "sizes.h"

namespace maplert {
// we may inline this?
// array-length: Returns the number of elements of the array
// referenced by 'p'
int32_t MCC_DexArrayLength(const void *p) {
  if (p == nullptr) {
    MRT_ThrowNullPointerExceptionUnw();
    return -1;
  }
  return *reinterpret_cast<const int32_t*>((reinterpret_cast<const char*>(p) + kJavaArrayLengthOffset));
}

int32_t MCC_JavaArrayLength(const void *p) __attribute__((alias("MCC_DexArrayLength")));

// we may inline this?
// fill-array-data: Fills the array referenced by d with the static data 's'.
void MCC_DexArrayFill(void *d, void *s, int32_t len) {
  __MRT_ASSERT(d && s, "MCC_DexArrayFill arg nullptr");
  char *dest = reinterpret_cast<char*>(d) + kJavaArrayContentOffset;
  size_t lenSizet = len;
  // We trust the frontend to supply the correct length
  errno_t rv = memcpy_s(dest, lenSizet, s, lenSizet);
  if (rv != EOK) {
    LOG(ERROR) << "memcpy_s failed.  Reason: " << rv << ", dest: " << dest << ", destMax: " <<
        lenSizet << ", src: " << s << ", count: " << lenSizet << ", SECUREC_MEM_MAX_LEN: " <<
        SECUREC_MEM_MAX_LEN << maple::endl;
    LOG(FATAL) << "check logcat and get more info " << maple::endl;
  }
}

void MCC_JavaArrayFill(void *d, void *s, int32_t len) __attribute__((alias("MCC_DexArrayFill")));

// we may move this to mpl2mpl where Java EH is lowered.
void *MCC_DexCheckCast(void *i __attribute__((unused)), void *c __attribute__((unused))) {
  // eventually, remove this builtin function.
  // and from the be lowerer
  __MRT_ASSERT(0, "MCC_DexCheckCast");
  return nullptr;
}

void *MCC_JavaCheckCast(void *i __attribute__((unused)), void *c __attribute__((unused)))
    __attribute__((alias("MCC_DexCheckCast")));

// Moves the class object of a class identified by type_id
// (e.g. Object.class) into vx.
void *MCC_GetReferenceToClass(void *p2tyid) {
  return p2tyid;
}

bool  MCC_DexInstanceOf(void *obj, void *javaClass) {
  if (UNLIKELY(obj == nullptr)) {
    return false;
  }
  MObject *o = reinterpret_cast<MObject*>(obj);
  MClass *c = reinterpret_cast<MClass*>(javaClass);
  DCHECK(c != nullptr) << "javaClass is nullptr." << maple::endl;
  return o->IsInstanceOf(*c);
}

bool MCC_JavaInstanceOf(void *obj, void *javaClass) __attribute__((alias("MCC_DexInstanceOf")));

bool MCC_IsAssignableFrom(jclass subClass, jclass superClass) {
  MClass *mSub = reinterpret_cast<MClass*>(subClass);
  MClass *mSuper = reinterpret_cast<MClass*>(superClass);
  DCHECK(mSub != nullptr) << "mSub is nullptr." << maple::endl;
  DCHECK(mSuper != nullptr) << "mSuper is nullptr." << maple::endl;
  return mSuper->IsAssignableFrom(*mSub);
}

void MCC_DexInterfaceCall(void *dummy __attribute__((unused))) {
  // placeholder for doing the right interface call at runtime
  __MRT_ASSERT(0, "MCC_DexInterfaceCall not implemented yet");
}

void MCC_JavaInterfaceCall(void *dummy __attribute__((unused))) __attribute__((alias("MCC_DexInterfaceCall")));

#if defined(__aarch64__)
jvalue MCC_DexPolymorphicCall(jstring calleeName, jstring protoString, int paramNum, jobject methodHandle, ...) {
  va_list args;
  va_start(args, methodHandle);
  VArg vargs(args);
  MClass *callerClass = GetContextCls(vargs, reinterpret_cast<MString*&>(calleeName));
  POLYRETURNTYPE result = PolymorphicCallEnter(reinterpret_cast<MString*>(calleeName),
                                               reinterpret_cast<MString*>(protoString), static_cast<uint32_t>(paramNum),
                                               reinterpret_cast<MObject*>(methodHandle), vargs, callerClass);
  va_end(args);
  return result;
}

jvalue MCC_JavaPolymorphicCall(jstring calleeName, jstring protoString, int paramNum, jobject methodHandle, ...)
    __attribute__((alias("MCC_DexPolymorphicCall")));
#endif

void MRT_BuiltinSyncEnter(address_t obj) {
  VLOG(monitor) << "MRT_BuiltinSyncEnter obj = " << obj << maple::endl;
  if (UNLIKELY(obj == 0)) {
    MRT_ThrowNullPointerExceptionUnw();
  }

  if (maple::IThread *tSelf = maple::IThread::Current()) {
    void *ra = __builtin_return_address(0);
    tSelf->PushLockedMonitor(reinterpret_cast<void*>(obj), ra, (uint32_t)maple::kMplWaitingOn);
    jint ret = maple::ObjectBase::MonitorEnter(obj);
    tSelf->UpdateLockMonitorState();
    if (UNLIKELY(ret == JNI_ERR)) {
      MRT_CheckThrowPendingExceptionUnw();
    }
  }
}

void MRT_BuiltinSyncExit(address_t obj) {
  VLOG(monitor) << "MRT_BuiltinSyncExit obj = " << obj << maple::endl;
  if (UNLIKELY(obj == 0)) {
    MRT_ThrowNullPointerExceptionUnw();
  }

  if (maple::IThread *tSelf = maple::IThread::Current()) {
    jint ret = maple::ObjectBase::MonitorExit(obj);
    tSelf->PopLockedMonitor();
    if (UNLIKELY(ret == JNI_ERR)) {
      MRT_CheckThrowPendingExceptionUnw();
    }
  }
}

void MRT_DumpMethodUse(std::ostream &os) {
  DumpMethodUse(os);
}

// Instrumentation/tracing facilities
static bool ResolveAddr2Name(std::string &funcname, uint64_t addr) {
  LinkerLocInfo info;
#ifdef RECORD_METHOD
  MRT_DisableMetaProfile();
#endif
  bool result = LinkerAPI::Instance().LocateAddress(reinterpret_cast<void*>(addr), info, true);
#ifdef RECORD_METHOD
  MRT_EnableMetaProfile();
#endif
  if (result) {
    funcname = info.sym;
    return true;
  }
  Dl_info dlInfo;
  if (dladdr(reinterpret_cast<void*>(addr), &dlInfo) && dlInfo.dli_sname) {
    funcname = dlInfo.dli_sname;
    return dlInfo.dli_sname;
  }
  return false;
}

#if defined(__aarch64__)
#define ENTRY_POINT_IMPL(name)                         \
    asm("    .text\n"                                    \
        "    .align 2\n"                                 \
        "    .globl __" #name                            \
        "__\n"                                           \
        "    .type  __" #name                            \
        "__, %function\n"                                \
        "__" #name                                       \
        "__:\n"                                          \
        " /* save arguments. must match frame below. */" \
        "    stp X29, X30, [sp,-16]!\n"                  \
        "    stp X0,  X1,  [SP,-16]!\n"                  \
        "    stp X2,  X3,  [SP,-16]!\n"                  \
        "    stp X4,  X5,  [SP,-16]!\n"                  \
        "    stp X6,  X7,  [SP,-16]!\n"                  \
        "    stp D0,  D1,  [SP,-16]!\n"                  \
        "    stp D2,  D3,  [SP,-16]!\n"                  \
        "    stp D4,  D5,  [SP,-16]!\n"                  \
        "    stp D6,  D7,  [SP,-16]!\n"                  \
        "    mov X0,  SP\n"                              \
        "    bl  " #name                                 \
        "Impl\n"                                        \
        "    ldp D6,  D7,  [SP], 16\n"                   \
        "    ldp D4,  D5,  [SP], 16\n"                   \
        "    ldp D2,  D3,  [SP], 16\n"                   \
        "    ldp D0,  D1,  [SP], 16\n"                   \
        "    ldp X6,  X7,  [SP], 16\n"                   \
        "    ldp X4,  X5,  [SP], 16\n"                   \
        "    ldp X2,  X3,  [SP], 16\n"                   \
        "    ldp X0,  X1,  [SP], 16\n"                   \
        "    ldp X29, X30, [SP], 16\n"                   \
        "    ret\n"); // this return send the control back to the original caller,
                      // not the next instruction of the jump instruction
#elif defined(__arm__)
#define ENTRY_POINT_IMPL(name)                         \
    asm("    .text\n"                                    \
        "    .align 2\n"                                 \
        "    .globl __" #name                            \
        "__\n"                                           \
        "    .type  __" #name                            \
        "__, %function\n"                                \
        "__" #name                                       \
        "__:\n"                                          \
        "\n");
#endif

#define ENTRY_POINT_BL(name) ENTRY_POINT_IMPL(name)
#define ENTRY_POINT_JMP(name) ENTRY_POINT_IMPL(name)

#ifdef __ANDROID__
// for save context and terminate system
asm("    .text\n"
    "    .align 2\n"
    "    .globl MRT_BuiltinAbortSaferegister\n"
    "    .type  MRT_BuiltinAbortSaferegister, %function\n"
    "MRT_BuiltinAbortSaferegister:\n"
#if defined(__aarch64__)
    "    brk #1\n"
    "    ret\n"
#endif
    "    .size    MRT_BuiltinAbortSaferegister, .-MRT_BuiltinAbortSaferegister");

#else
extern "C" void MRT_BuiltinAbortSaferegister(maple::address_t addr ATTR_UNUSED, const char *clsName ATTR_UNUSED) {
  util::PrintBacktrace();
  abort();
}
#endif

struct AArch64RegSet {
  uint64_t x0;
  uint64_t x1;
  uint64_t x2;
  uint64_t x3;
  uint64_t x4;
  uint64_t x5;
  uint64_t x6;
  uint64_t x7;
  uint64_t x8;
  uint64_t x9;
  uint64_t x10;
  uint64_t x11;
  uint64_t x12;
  uint64_t x13;
  uint64_t x14;
  uint64_t x15;
  uint64_t x16;
  uint64_t x17;
  uint64_t x18;
  uint64_t x19;
  uint64_t x20;
  uint64_t x21;
  uint64_t x22;
  uint64_t x23;
  uint64_t x24;
  uint64_t x25;
  uint64_t x26;
  uint64_t x27;
  uint64_t x28;
  uint64_t x29;
  uint64_t x30;
  uint64_t sp;
};

const int kMaxRegConst = 128;
static void PrintRegister(const std::string reg, uint64_t val) {
  char content[kMaxRegConst];
  int ret = sprintf_s(content, sizeof(content), "\t - phy reg %s\t%zx\t%zu", reg.c_str(), val, val);
  if (ret < EOK) {
    LOG(ERROR) << "sprintf_s failed. buffer: " << content << maple::endl;
    return;
  }
#ifdef __ANDROID__
  LOG(INFO) << content << maple::endl;
#else
  printf("%s\n", content);
#endif
}

extern "C" void PrintAArch64RegSet(AArch64RegSet *pregs) {
  const char *msg = "dump phy reg for aarch64, note x16 and x17 are unreliable due to plt or signal handler stub";
#ifdef __ANDROID__
  LOG(INFO) << msg << maple::endl;
#else
  printf("%s\n", msg);
#endif
  DCHECK(pregs != nullptr) << "pregs is nullptr in PrintAArch64RegSet" << maple::endl;
  PrintRegister("x0", pregs->x0);
  PrintRegister("x1", pregs->x1);
  PrintRegister("x2", pregs->x2);
  PrintRegister("x3", pregs->x3);
  PrintRegister("x4", pregs->x4);
  PrintRegister("x5", pregs->x5);
  PrintRegister("x6", pregs->x6);
  PrintRegister("x7", pregs->x7);
  PrintRegister("x8", pregs->x8);
  PrintRegister("x9", pregs->x9);
  PrintRegister("x10", pregs->x10);
  PrintRegister("x11", pregs->x11);
  PrintRegister("x12", pregs->x12);
  PrintRegister("x13", pregs->x13);
  PrintRegister("x14", pregs->x14);
  PrintRegister("x15", pregs->x15);
  PrintRegister("x16", pregs->x16);
  PrintRegister("x17", pregs->x17);
  PrintRegister("x18", pregs->x18);
  PrintRegister("x19", pregs->x19);
  PrintRegister("x20", pregs->x20);
  PrintRegister("x21", pregs->x21);
  PrintRegister("x22", pregs->x22);
  PrintRegister("x23", pregs->x23);
  PrintRegister("x24", pregs->x24);
  PrintRegister("x25", pregs->x25);
  PrintRegister("x26", pregs->x26);
  PrintRegister("x27", pregs->x27);
  PrintRegister("x28", pregs->x28);
  PrintRegister("x29", pregs->x29);
  PrintRegister("x30", pregs->x30);
  PrintRegister("sp", pregs->sp);

  // dump anything of interest here
}

// undocumented entry points
ENTRY_POINT_BL(MplDtEnter)
ENTRY_POINT_JMP(MplDtExit)
ENTRY_POINT_JMP(MplFuncProfile)
ENTRY_POINT_BL(MplTraceWithBreak)

struct frame {
  double d6;
  double d7;
  double d4;
  double d5;
  double d2;
  double d3;
  double d0;
  double d1;
  uint64_t x6;
  uint64_t x7;
  uint64_t x4;
  uint64_t x5;
  uint64_t x2;
  uint64_t x3;
  uint64_t x0;
  uint64_t x1;
  uint64_t x29;  // FP
  uint64_t x30;  // LR
};

extern "C" {
const unsigned int kNumLimit = 10;
const unsigned int kCodeOffset = 4;
void DecodeName(const char *name, char *dname) {
  size_t i = 0;
  size_t j = 0;

  if (name == nullptr || dname == nullptr) {
    return;
  }
  size_t len = strlen(name);
  const size_t maxLen = 512;
  if (len >= maxLen) {
    return;
  }

  while (i < len) {
    unsigned char c = name[i++];
    if (c == '_') {
      if (name[i] == '_') {
        dname[j++] = name[i++];
      } else {
        // _XX: '_' followed by ascii code in hex
        c = name[i++];
        auto v = static_cast<unsigned>((c <= '9') ? c - '0' : c - 'A' + kNumLimit);
        unsigned asc = v << kCodeOffset;
        c = name[i++];
        v = static_cast<unsigned>((c <= '9') ? c - '0' : c - 'A' + kNumLimit);
        asc += v;
        dname[j++] = static_cast<char>(asc);

        if (asc == '(' || asc == ')' || asc == ';') {
          while (name[i] == 'A') {
            dname[j++] = '[';
            i++;
          }
        }
      }
    } else {
      dname[j++] = c;
    }
  }
  dname[j] = '\0';
  return;
}

static __attribute__((used)) __thread int nEntered = 0;
const int kStackCapacity = 1024;
static __attribute__((used)) __thread uint64_t funcStack[kStackCapacity];

void MplFuncProfileImpl(struct frame *fr) {
  if (!VLOG_IS_ON(methodtrace)) {
    return;
  }
  std::string func("unknown");
  std::string soname("unknown");
  std::ostringstream funcaddrstr;
  DCHECK(fr != nullptr) << "fr is nullptr in MplFuncProfileImpl" << maple::endl;
  uint64_t faddr = fr->x30 - 4; /* 4 for bl to this call */
  if (CheckMethodResolved(faddr)) {
    return;
  }
#if RECORD_FUNC_NAME
  bool result = resolve_addr2name(func, faddr);
  if (!result) {
    LOG(WARNING) << "resolve " << std::hex << "0x" << faddr << " failed " << std::dec;
    funcaddrstr << std::hex << "0x" << faddr;
    func = funcaddrstr.str();
  } else {
    soname = maple::LinkerAPI::Instance().GetMFileNameByPC(reinterpret_cast<void*>(faddr), false);
  }
#endif
  RecordMethod(faddr, func, soname);
}

// No floating point can be used in this function.
__attribute__((used)) void MplDtEnterImpl(struct frame *fr) {
  if (!VLOG_IS_ON(methodtrace)) {
    return;
  }
  std::string func("unknown");
  std::ostringstream funcaddrstr;
  DCHECK(fr != nullptr) << "fr is nullptr in MplDtEnterImpl" << maple::endl;
  uint64_t faddr = fr->x30 - 4; // 4 for bl to this call
  bool result = ResolveAddr2Name(func, faddr);
  if (!result) {
    LOG(WARNING) << "resolve " << std::hex << "0x" << faddr << " failed " << std::dec;
  }
  pthread_t self;
  self = pthread_self();
  char dfunc[512]; // max length of dfunc
  int top = nEntered++;
  if (top < kStackCapacity && top >= 0) {
    funcStack[top] = faddr;
  }
  DecodeName(func.c_str(), dfunc);
  (void)fprintf(stderr, "enter: [%d:%p] %s : %16p\n", top, reinterpret_cast<void*>(self), dfunc,
                reinterpret_cast<void*>(fr->x0));
#if CONFIG_TRACE
  const int minDFuncLength = 12;
  if (IsTracingEnabled()) {
    if (strlen(dfunc) <= minDFuncLength) {
      return;
    }

    GetTracer()->LogMethodTraceEvent(dfunc, 0x0);
  }
#endif
}

__attribute__((used)) void MplDtExitImpl(struct frame *fr) {
  if (!VLOG_IS_ON(methodtrace)) {
    return;
  }

  pthread_t self;
  self = pthread_self();
  std::string func;
  char dfunc[512] = { 0 }; // max length of dfunc
  uint64_t faddr = 0;
  int top = --nEntered;
  if (top < kStackCapacity && top >= 0) {
    faddr = funcStack[top];
  }
  if (faddr != 0) {
    if (ResolveAddr2Name(func, faddr)) {
      DecodeName(func.c_str(), dfunc);
    }
  }
  DCHECK(fr != nullptr) << "fr is nullptr in MplDtExitImpl" << maple::endl;
  fprintf(stderr, " exit: [%d:%p] %s : rv %16p\n", top, reinterpret_cast<void*>(self),
          (faddr != 0) ? dfunc : "unknown", reinterpret_cast<void*>(fr->x0));
#if CONFIG_TRACE
  const int minDfuncLength = 12;
  if (IsTracingEnabled()) {
    if (strlen(dfunc) <= minDfuncLength) {
      return;
    }

    GetTracer()->LogMethodTraceEvent(dfunc, 0x1);
  }
#endif
}
const int kAddrRegOffset = 8;
// No floating point can be used in this function.
__attribute__((used)) void MplTraceImpl(struct frame *fr) {
  std::string func;
  char dfunc[512]; // max length of dfunc
  DCHECK(fr != nullptr) << "fr is nullptr in MplTraceImpl" << maple::endl;
  if (ResolveAddr2Name(func, fr->x30 - kAddrRegOffset)) {
    DecodeName(func.c_str(), dfunc);
    (void)fprintf(stderr, "%s : %16p\n", dfunc, reinterpret_cast<void*>(fr->x0));
  }
}

__attribute__((noinline)) void SetBreakPoint() {
}

// No floating point can be used in this function.
__attribute__((used)) void MplTraceWithBreakImpl(struct frame *fr) {
  MplTraceImpl(fr);
  SetBreakPoint();
}
}

}  // namespace maplert
