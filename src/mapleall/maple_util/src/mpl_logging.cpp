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
#include "mpl_logging.h"
#include <unistd.h>
#include <cstring>
#include <ctime>
#ifndef _WIN32
#include <sys/syscall.h>
#endif
#include "securec.h"

namespace {
constexpr uint32_t kMaxLogLen = 10000;
}

namespace maple {
LogInfo logInfo;
LogInfo &log = logInfo;

const char *kLongLogLevels[] = { [kLlDbg] = "D",      [kLlLog] = "L",     [kLlInfo] = "Info ",
                                 [kLlWarn] = "Warn ", [kLlErr] = "Error", [kLlFatal] = "Fatal" };

const char *tags[] = {
    [kLtThread] = "TR",
    [kLtLooper] = "LP",
};

SECUREC_ATTRIBUTE(7, 8) void LogInfo::EmitLogForDevelop(enum LogTags tag, enum LogLevel ll, const std::string &file,
                                                        const std::string &func, int line, const char *fmt, ...) const {
  char buf[kMaxLogLen];
  ASSERT(tag < kLtAll, "illegal log tag");

#ifndef _WIN32
  pid_t tid = syscall(SYS_gettid);
#endif

  time_t timeSeconds = time(nullptr);
  struct tm *nowTime = localtime(&timeSeconds);
  CHECK_FATAL(nowTime != nullptr, "(nowTime) null ptr check");
  int month = nowTime->tm_mon + 1;
  int day = nowTime->tm_mday;
  int hour = nowTime->tm_hour;
  int min = nowTime->tm_min;
  int sec = nowTime->tm_sec;

#ifdef _WIN32
  int lenFront = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "%02d-%02d %02d:%02d:%02d %s %s ",
                            month, day, hour, min, sec, tags[tag], kLongLogLevels[ll]);
#else
  int lenFront = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "%02d-%02d %02d:%02d:%02d %s %d %s ",
                            month, day, hour, min, sec, tags[tag], tid, kLongLogLevels[ll]);
#endif

  if (lenFront == -1) {
    WARN(kLncWarn, "snprintf_s failed in mpl_logging.cpp");
    return;
  }
  va_list l;
  va_start(l, fmt);

  int lenBack = vsnprintf_s(buf + lenFront, kMaxLogLen - static_cast<uint32_t>(lenFront),
                            kMaxLogLen - static_cast<uint32_t>(lenFront) - 1, fmt, l);
  if (lenBack == -1) {
    WARN(kLncWarn, "vsnprintf_s  failed ");
    va_end(l);
    return;
  }
  if (outMode != 0) {
    int eNum = snprintf_s(buf + lenFront + lenBack,
                          kMaxLogLen - static_cast<uint32_t>(lenFront) - static_cast<uint32_t>(lenBack),
                          kMaxLogLen - static_cast<uint32_t>(lenFront - lenBack) - 1,
                          " [%s] [%s:%d]", func.c_str(), file.c_str(), line);
    if (eNum == -1) {
      WARN(kLncWarn, "snprintf_s failed");
      va_end(l);
      return;
    }
  } else {
    int eNum = snprintf_s(buf + lenFront + lenBack,
                          static_cast<size_t>(kMaxLogLen - static_cast<uint32_t>(lenFront - lenBack)),
                          static_cast<size_t>(kMaxLogLen - static_cast<uint32_t>(lenFront - lenBack) - 1),
                          " [%s]", func.c_str());
    if (eNum == -1) {
      WARN(kLncWarn, "snprintf_s failed");
      va_end(l);
      return;
    }
  }
  va_end(l);
  (void)fprintf(outStream, "%s\n", buf);
  return;
}

SECUREC_ATTRIBUTE(4, 5) std::string LogInfo::EmitLogToStringForUser(enum LogNumberCode num, enum LogLevel ll,
                                                                    const char *fmt, ...) const {
  char buf[kMaxLogLen];
  int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "%s %02d: ", kLongLogLevels[ll], num);
  if (len == -1) {
    WARN(kLncWarn, "snprintf_s failed");
    return "";
  }
  if (outMode != 0) {
    va_list l;
    va_start(l, fmt);
    int eNum = vsnprintf_s(buf + len, kMaxLogLen - len, static_cast<size_t>(kMaxLogLen - len - 1), fmt, l);
    if (eNum == -1) {
      WARN(kLncWarn, "vsnprintf_s failed");
      va_end(l);
      return "";
    }
    va_end(l);
  }
  std::string logStr(buf);
  return logStr;
}

SECUREC_ATTRIBUTE(4, 5) void LogInfo::EmitLogForUser(enum LogNumberCode num, enum LogLevel ll, const char *fmt,
                                                     ...) const {
  char buf[kMaxLogLen];
  int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "%s %02d: ", kLongLogLevels[ll], num);
  if (len == -1) {
    WARN(kLncWarn, "snprintf_s failed");
    return;
  }
  if (outMode != 0) {
    va_list l;
    va_start(l, fmt);
    int eNum = vsnprintf_s(buf + len, kMaxLogLen - len, static_cast<size_t>(kMaxLogLen - len - 1), fmt, l);
    if (eNum == -1) {
      WARN(kLncWarn, "vsnprintf_s failed");
      va_end(l);
      return;
    }
    va_end(l);
  }
  std::cerr << buf << '\n';
  return;
}

void LogInfo::EmitLogForUser(enum LogNumberCode num, enum LogLevel ll, const std::string &message) const {
  EmitLogForUser(num, ll, "%s", message.c_str());
}

SECUREC_ATTRIBUTE(5, 6) void LogInfo::EmitErrorMessage(const std::string &cond, const std::string &file,
                                                       unsigned int line, const char *fmt, ...) const {
  char buf[kMaxLogLen];
#ifdef _WIN32
  int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "CHECK/CHECK_FATAL failure: %s at [%s:%u] ",
                       cond.c_str(), file.c_str(), line);
#else
  pid_t tid = syscall(SYS_gettid);
  int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "Tid(%d): CHECK/CHECK_FATAL failure: %s at [%s:%u] ",
                       tid, cond.c_str(), file.c_str(), line);
#endif
  if (len == -1) {
    WARN(kLncWarn, "snprintf_s failed");
    return;
  }
  va_list l;
  va_start(l, fmt);
  int eNum = vsnprintf_s(buf + len, kMaxLogLen - len, static_cast<size_t>(kMaxLogLen - len - 1), fmt, l);
  if (eNum == -1) {
    WARN(kLncWarn, "vsnprintf_s failed");
    va_end(l);
    return;
  }
  va_end(l);
  std::cerr << buf;
}

std::ios::fmtflags LogInfo::Flags() {
  std::ios::fmtflags flag(std::cout.flags());
  return std::cout.flags(flag);
}

std::ostream &LogInfo::MapleLogger(LogLevel level) {
  if (level == kLlLog) {
    return std::cout;
  } else {
    return std::cerr;
  }
}
std::ostream &LogInfo::Info() {
  return std::cout;
}
std::ostream &LogInfo::Err() {
  return std::cerr;
}

void LogInfo::InsertUserWarnMessage(const Loc &loc, const std::string &message) {
  std::lock_guard<std::mutex> lk(warnsMtx);
  userWarnsMap[loc].emplace_back(message);
  ++warnsNum;
}

void LogInfo::PrintUserWarnMessages() {
  std::lock_guard<std::mutex> lk(warnsMtx);
  for (auto it = userWarnsMap.cbegin(); it != userWarnsMap.cend(); ++it) {
    std::vector<std::string> warns = it->second;
    for (size_t i = 0; i < warns.size(); ++i) {
      std::cerr << warns[i] << '\n';
    }
  }
  if (!userWarnsMap.empty()) {
    INFO(kLncInfo, "%d warn(s) generated.", warnsNum);
    userWarnsMap.clear();
  }
}

void LogInfo::InsertUserErrorMessage(const Loc &loc, const std::string &err) {
  std::lock_guard<std::mutex> lk(errsMtx);
  userErrsMap[loc].emplace_back(err);
  ++errsNum;
}

void LogInfo::PrintUserErrorMessages() {
  std::lock_guard<std::mutex> lk(errsMtx);
  for (auto it = userErrsMap.cbegin(); it != userErrsMap.cend(); ++it) {
    std::vector<std::string> errs = it->second;
    for (size_t i = 0; i < errs.size(); ++i) {
      std::cerr << errs[i] << '\n';
    }
  }
  if (!userErrsMap.empty()) {
    INFO(kLncInfo, "%d error(s) generated.", errsNum);
    userErrsMap.clear();
  }
}

std::string GetNthStr(size_t index) {
  switch (index) {
    case 0:
      return "1st";
    case 1:
      return "2nd";
    case 2:
      return "3rd";
    default: {
      std::ostringstream oss;
      oss << (index + 1) << "th";
      return oss.str();
    }
  }
}
}  // namespace maple
