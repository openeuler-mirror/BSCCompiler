/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * c++ utils library is licensed under Mulan PSL v2.
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
#ifndef MAPLE_RUNTIME_TIME_UTILS_H
#define MAPLE_RUNTIME_TIME_UTILS_H

#include <string>
#include <cinttypes>

namespace timeutils {

// returns the monotonic time since epoch starting point in milliseconds
uint64_t MilliSeconds();

// returns the monotonic time since epoch starting point in nanoseconds
uint64_t NanoSeconds();

// returns the monotonic time since epoch starting point in microseconds
uint64_t MicroSeconds() noexcept;

// returns the thread-specific cpu clock time in nanoseconds or -1 if the clock is not exist
uint64_t ThreadCpuTimeNs();

// returns the process cpu clock time in nanoseconds or -1 if the clock is not exist
uint64_t ProcessCpuTimeNs();

// sleep for the given count of nanoseconds
void SleepForNano(uint64_t ns);

// Sleep forever
void SleepForever();

// returns the current date in ios yyymmddhhmmss format
std::string GetDigitDate();

// returns the current date in ios yyyy-mm-dd hh:mm::ss format
std::string GetIsoDate();

void ConstructTimeSpec(int clock, int64_t millSeconds, int32_t nanoSeconds, timespec &ts, bool absolute);

// if time is at future, now remain time is update to 'updateTime'
// if time has past, it will return false;
bool UpdateRemainTimeToNow(int clock, const timespec &time, timespec &updateTime);

} // namespace timeutils
#endif // MAPLE_TIME_UTILS_H
