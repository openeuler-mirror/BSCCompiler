/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_UTIL_INCLUDE_MPL_TIMER_H
#define MAPLE_UTIL_INCLUDE_MPL_TIMER_H

#include <chrono>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

namespace maple {
class MPLTimer {
 public:
  MPLTimer();
  ~MPLTimer();
  void Start();
  void Stop();
  long Elapsed() const;
  long ElapsedMilliseconds() const;
  long ElapsedMicroseconds() const;
 private:
  std::chrono::system_clock::time_point startTime;
  std::chrono::system_clock::time_point endTime;
};

class MPLTimerManager {
 public:
  enum class Unit {
    kMicroSeconds,  // us
    kMilliSeconds,  // ms
    kSeconds,       // s
  };

  // time data
  struct Timer {
    void Start() noexcept {
      startTime = std::chrono::system_clock::now();
      ++count;
    }

    void Stop() noexcept {
      useTime += (std::chrono::system_clock::now() - startTime);
    }

    template<class T = std::chrono::milliseconds>
    long Elapsed() const noexcept {
      return std::chrono::duration_cast<T>(useTime).count();
    }

    std::chrono::system_clock::time_point startTime;
    std::chrono::nanoseconds useTime;
    uint32_t count = 0;   // run count
  };

  MPLTimerManager() = default;
  virtual ~MPLTimerManager() = default;

  void Clear() {
    allTimer.clear();
  }

  Timer &GetTimerFormKey(const std::string &key) {
    return allTimer[key];
  }

  template<Unit unit = Unit::kMilliSeconds>
  std::string ConvertAllTimer2Str() const {
    std::ostringstream os;
    for (auto &[key, timer] : allTimer) {
      os << "\t" << key << ": ";
      if constexpr (unit == Unit::kMicroSeconds) {
        os << timer.Elapsed<std::chrono::microseconds>() << "us";
      } else if constexpr (unit == Unit::kMilliSeconds) {
        os << timer.Elapsed<std::chrono::milliseconds>() << "ms";
      } else {
        static_assert(unit == Unit::kSeconds, "unknown units");
        os << timer.Elapsed<std::chrono::seconds>() << "s";
      }
      os << ", count: " << timer.count << std::endl;
    }
    return os.str();
  }
 private:
  std::map<std::string, Timer> allTimer;
};

class MPLTimerRegister {
 public:
  MPLTimerRegister(MPLTimerManager &timerM, const std::string &key) {
    timer = &timerM.GetTimerFormKey(key);
    timer->Start();
  }

  ~MPLTimerRegister() {
    Stop();
  }

  void Stop() noexcept {
    if (timer != nullptr) {
      timer->Stop();
      timer = nullptr;
    }
  }
 private:
  MPLTimerManager::Timer *timer = nullptr;
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_MPL_TIMER_H
