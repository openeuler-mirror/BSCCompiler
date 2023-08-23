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
#ifndef MPL_RUNTIME_INTERPRETER_SUPPORT_H
#define MPL_RUNTIME_INTERPRETER_SUPPORT_H

#include "exception/stack_unwinder.h"
#include "methodmeta.h"
#include "loader_api.h"
#include "chelper.h"
// forward declaring
namespace maple {
struct maple_native_stack_item;
}

namespace maplert {
inline bool MethodMeta::NeedsInterp() const {
  return false;
}

extern "C" inline bool TryGetNativeContexClassLoaderForInterp(const UnwindContext &, jclass &) {
  return false;
}

class MClassLocatorManagerInterpEx {
 public:
  static inline bool LoadClasses(LoaderAPI &, jobject, ObjFile &) {
    return false;
  }

  static inline bool RegisterNativeMethods(LoaderAPI &, ObjFile &, jclass, const JNINativeMethod *, jint) {
    return false;
  }
};

class MClassLocatorInterpEx {
 public:
  static inline ClassMetadata *BuildClassFromDex(IObjectLocator, const std::string &, SearchFilter &) {
    return nullptr;
  }
};

class UnwindContextInterpEx {
 public:
  static inline UnwindState UnwindToCaller(void *, UnwindContext &) {
    return kUnwindSucc;
  }

  static inline void VisitGCRoot(UnwindContext &, const AddressVisitor &) {
    return;
  }

  static inline bool TryRecordStackFromUnwindContext(std::vector<uint64_t> &, UnwindContext &) {
    return false;
  }

  static inline bool TryDumpStackInfoInLog(LinkerAPI &, UnwindContext &) {
    return false;
  }

  MRT_EXPORT static inline MClass *GetDeclaringClassFromUnwindContext(UnwindContext &) {
    return nullptr;
  }

  MRT_EXPORT static inline void GetJavaMethodFullNameFromUnwindContext(UnwindContext &, std::string &) {
    return;
  }

  MRT_EXPORT static inline void LogCallerClassInfoFromUnwindContext(UnwindContext &, UnwindContext &, int &) {
    return;
  }

  MRT_EXPORT static inline void LogCallerFilenameFromUnwindContext(UnwindContext &, int &) {
    return;
  }

  MRT_EXPORT static inline void GetStackFrameFromUnwindContext(UnwindContext &, maple::maple_native_stack_item &) {
    return;
  }
};

class InterpSupport {
 public:
  static inline std::string GetSoName() {
    return std::string("unknown");
  }
};

namespace interpreter {
static inline std::string GetStringFromInterpreterStrTable(int32_t) {
  return std::string();
}

template<calljavastubconst::ArgsType argsType>
static inline jvalue InterpJavaMethod(MethodMeta *, MObject *, uintptr_t) {
  jvalue ret;
  ret.j = 0LL;
  return ret;
}

template<typename T, calljavastubconst::ArgsType argsType>
static inline T InterpJavaMethod(MethodMeta *, MObject *, const uintptr_t) {
  return (T)0;
}
} // namespace interpreter
} // namespace maplert

#endif // MPL_RUNTIME_INTERPRETER_SUPPORT_H
