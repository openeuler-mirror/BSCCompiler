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

#ifndef MAPLE_RUNTIME_BASE_LOGGING_H_
#define MAPLE_RUNTIME_BASE_LOGGING_H_

#include <ostream>
#include <sstream>
#include <functional>
#include <memory>
#include <string>
#include <sys/uio.h>
#include <cstring>
#include <cerrno>
#include "base/macros.h"

#ifndef LOG_TAG
#define LOG_TAG "maple"
#endif

namespace maple {

enum LogId {
  DEFAULT,
  MAIN,
  SYSTEM,
};

enum LogLevel {
  VERBOSE,
  DEBUGY,
  INFO,
  WARNING,
  ERROR,
  FATAL_WITHOUT_ABORT,
  FATAL,
};

void InitLogging();

#define LEVEL_LAMBDA(level) ([&]() {  \
  using ::maple::VERBOSE;             \
  using ::maple::DEBUGY;              \
  using ::maple::INFO;                \
  using ::maple::WARNING;             \
  using ::maple::ERROR;               \
  using ::maple::FATAL_WITHOUT_ABORT; \
  using ::maple::FATAL;               \
  return (level); }())

#define WOULD_LOG(level) \
  (UNLIKELY((LEVEL_LAMBDA(level)) >= ::maple::GetMinimumLogLevel()))

#define LOG_STREAM(level)                                           \
  ::maple::LogData(__FILE__, __LINE__,                              \
                              LEVEL_LAMBDA(level), LOG_TAG)         \
      .stream()

#define LOG(level) if (WOULD_LOG(level))  LOG_STREAM(level)

#define PLOG(level) LOG(level)

#define VLOG_IS_ON(module) (::maple::gLogVerbosity.module)

#define VLOG(module) if (VLOG_IS_ON(module)) LOG(INFO)

#ifndef LOG_NDEBUG
#define LOG_NDEBUG 1
#endif

#define DLOG(module) if (((LOG_NDEBUG == 1) ? false : VLOG_IS_ON(module))) LOG(DEBUGY)

#ifndef NEVERCALLTHIS
#define NEVERCALLTHIS(level) LOG(level)
#endif // NEVERCALLTHIS

#define UNIMPLEMENTED(level) \
  LOG(level) << __PRETTY_FUNCTION__ << " unimplemented "

#define CHECK(x)                                           \
  if (!(LIKELY((x))))                                      \
      ::maple::LogData(__FILE__, __LINE__,                 \
                                  ::maple::FATAL, LOG_TAG) \
              .stream()                                    \
          << "Check failed: " #x << " "

#define CHECK_E(x, str)                  \
  do {                                   \
    if (x) {                             \
      LOG(ERROR) << str << maple::endl;  \
    }                                    \
  } while (false);

#define CHECK_E_B(x, str)                \
  do {                                   \
    if (x) {                             \
      LOG(ERROR) << str << maple::endl;  \
      return false;                      \
    }                                    \
  } while (false);

#define CHECK_E_V(x, str)                \
  do {                                   \
    if (x) {                             \
      LOG(ERROR) << str << maple::endl;  \
      return;                            \
    }                                    \
  } while (false);

#define CHECK_E_P(x, str)                \
  do {                                   \
    if (x) {                             \
      LOG(ERROR) << str << maple::endl;  \
      return nullptr;                    \
    }                                    \
  } while (false);


#define CHECK_OP(LHS, RHS, OP)                                                                       \
  for (auto _values = ::maple::CreateValueHolder(LHS, RHS);                                          \
       UNLIKELY(!(_values.GetLHS() OP _values.GetRHS()));                                            \
       /* empty */)                                                                                  \
  ::maple::LogData(__FILE__, __LINE__,                                                               \
                              ::maple::FATAL, LOG_TAG)                                               \
          .stream()                                                                                  \
      << "Check failed: " << #LHS << " " << #OP << " " << #RHS << " (" #LHS "=" << _values.GetLHS()  \
      << ", " #RHS "=" << _values.GetRHS()<< ") "

#define CHECK_EQ(x, y) CHECK_OP(x, y, ==)
#define CHECK_NE(x, y) CHECK_OP(x, y, !=)
#define CHECK_GE(x, y) CHECK_OP(x, y, >=)
#define CHECK_GT(x, y) CHECK_OP(x, y, >)
#define CHECK_LE(x, y) CHECK_OP(x, y, <=)
#define CHECK_LT(x, y) CHECK_OP(x, y, <)

#define CHECK_PTHREAD_CALL(call, args, what)                           \
  do {                                                                 \
    int rc = call args;                                                \
    if (rc != 0) {                                                     \
      errno = rc;                                                      \
      PLOG(FATAL) << #call << " failed for " << (what) << " reason "   \
                  << strerror(errno) << " return " << errno;           \
    }                                                                  \
  } while (false)

#if defined(NDEBUG)
static constexpr bool kEnableDebugChecks = false;
#else
static constexpr bool kEnableDebugChecks = true;
#endif

#define DCHECK(x) \
  if (::maple::kEnableDebugChecks) CHECK(x)
#define DCHECK_EQ(x, y) \
  if (::maple::kEnableDebugChecks) CHECK_EQ(x, y)
#define DCHECK_NE(x, y) \
  if (::maple::kEnableDebugChecks) CHECK_NE(x, y)
#define DCHECK_LE(x, y) \
  if (::maple::kEnableDebugChecks) CHECK_LE(x, y)
#define DCHECK_LT(x, y) \
  if (::maple::kEnableDebugChecks) CHECK_LT(x, y)
#define DCHECK_GE(x, y) \
  if (::maple::kEnableDebugChecks) CHECK_GE(x, y)
#define DCHECK_GT(x, y) \
  if (::maple::kEnableDebugChecks) CHECK_GT(x, y)

template <typename LHS, typename RHS>
class ValueHolder {
 public:
  constexpr ValueHolder(LHS l, RHS r);

  LHS GetLHS();
  RHS GetRHS();

 private:
  LHS lhs;
  RHS rhs;
};

template <typename LHS, typename RHS>
ValueHolder<LHS, RHS> CreateValueHolder(LHS lhs, RHS rhs);

class LogData {
 public:
  LogData(const char* file, unsigned int line, LogLevel level, const char* tag);

  ~LogData();

  std::ostream& stream();

 private:
#if defined(__ANDROID__) || defined(__ZIDANE__)
  std::ostringstream oStringBuffer;
#endif
  const char* const fileName;
  const unsigned int lineNumber;
  const LogLevel logLevel;
  const char* const logTag;
};

LogLevel GetMinimumLogLevel();

// The members of this struct are the valid arguments to VLOG and VLOG_IS_ON in code,
// and the "-verbose:" command line argument.
struct LogVerbosity {
  bool jni;
  bool binding;
  bool startup;
  bool jvm;
  bool thread;
  bool classloader;
  bool mpllinker;
  bool lazybinding;
  bool hotfix;
  bool signals;
  bool gc;
  bool rc;
  bool eh;
  bool stackunwind;
  bool reflect;
  bool methodtrace;
  bool classinit;
  bool bytecodeverify;
  bool printf;
  bool monitor;
  bool profiler;
  bool rcreleasetrace;
  bool jsan;
  bool nativeleak;
  bool addr2line;
  bool systrace_lock_logging;  // Enabled with "-verbose:sys-locks".
  // for rc backup tracing debug
  bool gcbindcpu;
  bool rcverify;
  bool immediaterp;
  bool opengclog;
  bool openrctracelog;
  bool openrplog;
  bool opencyclelog;
  bool opennativelog;
  bool allocatorfragmentlog;
  bool allocatorlog;
  bool openmixlog;
  bool dumpgarbage;
  bool dumpheap;
  bool dumpheapbeforegc;
  bool dumpheapaftergc;
  bool dumpheapsimple;
  bool dumpgcinfoonsigquit;
  bool jdwp;
  bool decouple;
  bool decoupleStats;
  bool decoupleCheck;
  bool staticdcp;
  bool decoupleNoCache;
  bool nonconcurrentgc;
  bool conservativestackscan;
  bool gconly;
  bool zterp;
  bool jsanlite;
};

extern LogVerbosity gLogVerbosity;
extern const char *const endl;
extern unsigned int abortingCnt;

extern void InitLogging(char* argv[]);

}  // namespace maple

#endif  // MAPLE_RUNTIME_BASE_LOGGING_H_
