/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_UTIL_INCLUDE_MPL_LOGGING_H
#define MAPLE_UTIL_INCLUDE_MPL_LOGGING_H

#include <string>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>

// This file defines the APIs that govern all messaging-styled output from
// a running program under MAPLE, which can be a compiler or a runtime component.
//
// There are two overall classes of messages:
//
// (1) Those of interest only to developers, and thus should be suppressed in the
// production release version of the software.  The message content will include
// the source file and line number of the trigger point of the message.
//
// (2) Those intended to be visible to users of the software in general,  in
// addition to the developers.
//
// Messages are divided into 6 types, or levels, from 0 to 5.  Conceptually,
// the lower the level, the higher the frequency of occurrences, the larger the
// output volume, the smaller the audience of interest and the more often they need
// to be filtered out. In addition, the higher the level, the greater the severity.
//
// Level 0 (DBG) - These are messages for debugging purposes, used by the
// developer during development to debug his code.
//
// Level 1 (LOG) - These are log messages, also for helping the developer in
// debugging, but at a higher level of algorithmic operation.
//
// Level 2 (INFO) - These provide information that are of general usefulness in
// the normal operation of the SW.
//
// Level 3 (WARN) - These provide warning messages.
//
// Level 4 (ERR) - These provide error messages.
//
// Level 5 (FATAL) - These indicate errors of such severity that the program r
// execution cannot continue.
//
// DBG and LOG are only for developers' use.  INFO, WARN, ERR and FATAL are
// intended for general visibility.  There is an additional type of ERR that
// designate developer errors that arise out of checking code inserted by the
// developers, which has the following 4 usage patterns:
//
// CHECK - If the specified program condition is not satisfied, output the error
// message. The program will continue execution.
//
// DCHECK - Same as CHECK, but the check is suppressed in the release version of
// the SW.
//
// CHECK_FATAL - If the specified program condition is not satisfied, output the error
// message. The program will stop execution at that point.
//
// ASSERT - Same as CHECK_FATAL, but the check is suppressed in the release version of
// the SW.
//
// The macro setting DEBUG=1 picks developer and DEBUG=0 picks release builds.
//
// the macro PRINT_LEVEL_DEV is a filter for DBG messages in developer builds.
// When PRINT_LEVEL_DEV is set to kLlLog, DBG messages are not printed.
//
// Instantiated object c_logInfo, of class LogInfo, provides finer control of
// the logging behavior during execution.  Use emitLogDevice() to change the
// message destination.  Use setLogMode() to change the verbosity of the messages.
//
// In the case of DBG and LOG, the message needs to print the name of the SW
// component as the volume of messages can be huge.  Use enum LOG_TAGS to define
// the component ID and its name string.
//
// Since INFO, WARN, ERR and FATAL are for general consumption, each message
// should provide an number code under enum LogNumberCode.
//
// Following are usage of logging actions supported:
//
// GDB,LOG,INFO,WARN,ERR,FATAL can be invoked as method.
//   parameters:
//     TAG
//     formatted string
//     variadic list
//
// CHECK,DCHECK,CHECK_FATAL,ASSERT also can be invoked as method.
//   parameters:
//     condition
//     formatted string
//     variadic list
//
// Each of the above are mapped to one of the following 3 methods in class LogInfo:
//
// EmitLogForDev() - for DBG and LOG
//
// EmitLogForUser() - for INFO, WARN, ERR and FATAL
//
// EmitErrorMessage() - for CHECK, DCHECK, CHECK_FATAL and ASSERT
//
// DBG and LOG send their messages to stdout, and provide additional date and time
// information.  For the rest, the messages are sent to stderr.
//
// In debugging the program, the developer can set breakpoint in one of the above
// 3 methods depending on the type of message. For ASSERT, abort() is called
// instead of exit(1) so that the program will not completely exit, to allow the
// developer to print stack trace and peruse the program environment at the point
// of the assertion.
namespace maple {
std::string GetNthStr(size_t index);

extern class LogInfo logInfo;
extern class LogInfo &log;

enum LogLevel {
  kLlDbg,
  kLlLog,
  kLlInfo,
  kLlWarn,
  kLlErr,
  kLlFatal,
  kLlMax
};

enum LogTags {
  kLtThread,
  kLtLooper,
  kLtAll
};

enum LogMode {
  kLmSimple,
  kLmComplex,
  kLmMax
};

enum LogNumberCode {
  kLncInfo = 0,
  kLncWarn = 20,
  kLncErr = 40,
  kLncFatal = 60,
  kLncMax = 99
};

struct Loc {
  std::uint32_t fileIdx;
  std::uint32_t line;
  std::uint32_t column;
  Loc(std::uint32_t fileIdxIn, std::uint32_t lineIn, std::uint32_t columnIn)
      : fileIdx(fileIdxIn),
        line(lineIn),
        column(columnIn) {}

  bool operator<(Loc const &loc) const {
    if (fileIdx != loc.fileIdx) {
      return fileIdx < loc.fileIdx;
    } else if (line != loc.line) {
      return line < loc.line;
    } else {
      return column < loc.column;
    }
  }

  bool operator==(Loc const &loc) const {
    return (fileIdx == loc.fileIdx) && (line == loc.line) && (column == loc.column);
  }

  std::string Dump() const {
    std::stringstream ss;
    ss << "fileIdx:" << fileIdx << ", line:" << line << ", column:" << column << "\n";
    return ss.str();
  }
};

class LogInfo {
 public:
  LogInfo() : outStream(stdout), outMode(kLmComplex) {}
  LogInfo(const LogInfo &p) = delete;
  static std::ostream &Info();
  static std::ostream &Err();
  LogInfo &operator=(const LogInfo &p) = delete;

  ~LogInfo() {
    fclose(outStream);
  }

  static std::ostream &MapleLogger(LogLevel level = kLlLog);
  static std::ios::fmtflags Flags();
  void EmitLogForUser(enum LogNumberCode num, enum LogLevel ll, const char *fmt, ...) const;
  std::string EmitLogToStringForUser(enum LogNumberCode num, enum LogLevel ll, const char *fmt, ...) const;
  void EmitLogForUser(enum LogNumberCode num, enum LogLevel ll, const std::string &message) const;
  void EmitErrorMessage(const std::string &cond, const std::string &file, unsigned int line,
                        const char *fmt, ...) const;
  void InsertUserWarnMessage(const Loc &loc, const std::string &message);
  void PrintUserWarnMessages();
  void InsertUserErrorMessage(const Loc &loc, const std::string &err);
  void PrintUserErrorMessages();
  uint GetUserWarnsNum() const {
    return warnsNum;
  }

  uint GetUserErrorsNum() const {
    return errsNum;
  }

 private:
  void SetLogDevice(FILE *stream) {
    outStream = stream;
  }
  void SetLogMode(LogMode lm) {
    outMode = lm;
  }
  void EmitLogForDevelop(enum LogTags tag, enum LogLevel ll, const std::string &file, const std::string &func,
                         int line, const char *fmt, ...) const;
  FILE *outStream;
  LogMode outMode;
  std::map<Loc, std::vector<std::string>> userErrsMap;
  std::map<Loc, std::vector<std::string>> userWarnsMap;
  uint errsNum = 0;
  uint warnsNum = 0;
  std::mutex warnsMtx;
  std::mutex errsMtx;
};

#ifdef DEBUG
#define DEBUG_STMT(x) x
#define DEBUG_TEST 1
#define ENABLE_ASSERT 1
#define EXCEPTION_QUIT abort()
#else
#define DEBUG_STMT(x)
#define DEBUG_TEST 0
#define ENABLE_ASSERT 0
#define EXCEPTION_QUIT exit(1)
#endif  // DEBUG

// for developer
#define PRINT_LEVEL_DEV kLlLog

#define DBG(tag, fmt, ...)                                                                      \
  do {                                                                                          \
    if (PRINT_LEVEL_DEV <= kLlLog) {                                                             \
      logInfo.EmitLogForDev(tag, kLlLog, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    }                                                                                           \
  } while (0)

#ifdef LOG
#undef LOG
#endif
#define LOG(tag, fmt, ...)                                                                      \
  do {                                                                                          \
    if (PRINT_LEVEL_DEV <= kLlLog) {                                                            \
      logInfo.EmitLogForDev(tag, kLlLog, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    }                                                                                                \
  } while (0)

#ifdef CHECK
#undef CHECK
#endif
#define CHECK(cond, fmt, ...)                                                       \
  do {                                                                              \
    if (!(cond)) {                                                                  \
      logInfo.EmitErrorMessage(#cond, __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__); \
    }                                                                                  \
  } while (0)

#ifdef DCHECK
#undef DCHECK
#endif
#define DCHECK(cond, fmt, ...)                   \
  do {                                           \
    DEBUG_STMT(CHECK(cond, fmt, ##__VA_ARGS__)); \
  } while (0)

// To shut down the codecheck warning: boolean condition for 'if' always evaluates to 'true'
#define CHECK_FATAL_FALSE(fmt, ...)                                                        \
  do {                                                                                     \
    maple::logInfo.EmitErrorMessage("false", __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__); \
    maple::logInfo.PrintUserWarnMessages();                                                \
    maple::logInfo.PrintUserErrorMessages();                                               \
    EXCEPTION_QUIT;                                                                        \
  } while (0)

#define CHECK_FATAL(cond, fmt, ...)                                                        \
  do {                                                                                     \
    if (!(cond)) {                                                                         \
      maple::logInfo.EmitErrorMessage(#cond, __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__); \
      maple::logInfo.PrintUserWarnMessages();                                              \
      maple::logInfo.PrintUserErrorMessages();                                             \
      EXCEPTION_QUIT;                                                                      \
    }                                                                                      \
  } while (0)

#define CHECK_NULL_FATAL(ptr) CHECK_FATAL((ptr) != nullptr, "Failed with nullptr.")

#if ENABLE_ASSERT
#define ASSERT(cond, fmt, ...)                                                      \
  do {                                                                              \
    if (!(cond)) {                                                                  \
      maple::logInfo.EmitErrorMessage(#cond, __FILE__, __LINE__, fmt "\n", ##__VA_ARGS__); \
      abort();                                                                      \
    }                                                                               \
  } while (0)

#define ASSERT_NOT_NULL(ptr) ASSERT(ptr != nullptr, "Failed with nullptr.")
#else
#define ASSERT(cond, fmt, ...)
#define ASSERT_NOT_NULL(ptr)
#endif  // ENABLE_ASSERT

// for user
#define PRINT_LEVEL_USER kLlInfo

#define INFO(num, fmt, ...)                                     \
  do {                                                          \
    if (PRINT_LEVEL_USER <= kLlInfo) {                          \
      logInfo.EmitLogForUser(num, kLlInfo, fmt, ##__VA_ARGS__); \
    }                                                              \
  } while (0)

#define INFO_V(verbose, num, fmt, ...)                          \
  if (verbose) {                                                \
    if (PRINT_LEVEL_USER <= kLlInfo) {                          \
      logInfo.EmitLogForUser(num, kLlInfo, fmt, ##__VA_ARGS__); \
    }                                                           \
  }

#define WARN_USER(num, pos, mod, fmt, ...)                                                                         \
  do {                                                                                                             \
    if (PRINT_LEVEL_USER <= kLlWarn) {                                                                             \
      std::ostringstream ss;                                                                                       \
      ss << mod.GetFileNameFromFileNum(pos.FileNum()) << ":" << pos.LineNum() << " warning: " << fmt;              \
      logInfo.InsertUserWarnMessage(                                                                               \
          pos.GetSrcLoc(), logInfo.EmitLogToStringForUser(num, kLlWarn, ss.str().c_str(), ##__VA_ARGS__));         \
    }                                                                                                              \
  } while (0)

#define WARN(num, fmt, ...)                                                                                        \
  do {                                                                                                             \
    if (PRINT_LEVEL_USER <= kLlWarn) {                                                                             \
      logInfo.EmitLogForUser(num, kLlWarn, fmt, ##__VA_ARGS__);                                                    \
    }                                                                                                              \
  } while (0)

#define ERR(num, fmt, ...)                                     \
  do {                                                         \
    if (PRINT_LEVEL_USER <= kLlErr) {                          \
      logInfo.EmitLogForUser(num, kLlErr, fmt, ##__VA_ARGS__); \
    }                                                          \
  } while (0)

#define FATAL(num, fmt, ...)                                     \
  do {                                                           \
    logInfo.PrintUserWarnMessages();                             \
    logInfo.PrintUserErrorMessages();                            \
    if (PRINT_LEVEL_USER <= kLlFatal) {                          \
      logInfo.EmitLogForUser(num, kLlFatal, fmt, ##__VA_ARGS__); \
    }                                                            \
    if (DEBUG_TEST != 0) {                                       \
      abort();                                                   \
    } else {                                                     \
      exit(1);                                                   \
    }                                                            \
  } while (0)
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_MPL_LOGGING_H
