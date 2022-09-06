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

#ifndef MAPLE_UTIL_INCLUDE_MPL_SIGHANDLER
#define MAPLE_UTIL_INCLUDE_MPL_SIGHANDLER

#include <csignal>
#include <map>

namespace maple {

class SigHandler {
 public:
  SigHandler() = delete;
  ~SigHandler(){};

  /// @brief enable handlig all supported signals
  static void EnableAll();
  /// @brief enable handlig signal "sig"
  static void EnableSig(int sig);

  /// @brief disable handling all supported signals
  static void DisableAll();
  /// @brief disable handling signal with num 'sig'
  static void DisableSig(int sig);

  /// @brief this function will be called in case that programm received signal
  static void Handler(int sig, siginfo_t *info, void *ucontext) noexcept;

  [[maybe_unused]] static void (*emptyFuncPtr)(int, siginfo_t *, void *) noexcept;
  using FuncPtr = decltype(emptyFuncPtr);

  /// @brief set user-defined signal callback
  /// 'callback' will be called at the end of handler function
  static void SetCallback(int sig, FuncPtr callback);

  /// @brief set alarm timer
  static void SetTimer(int seconds);

  /// @return true if signal is supported, otherwise -- false
  static bool IsSupportedSig(int sig);

 private:
  static void SetSigaction(int sig, FuncPtr callback);
  static void SetDefaultSigaction(int sig);

  // map of user-defined callbacks
  static std::map<int, FuncPtr> sig2callback;
};
}  // namespace maple

#endif  // MAPLE_UTIL_INCLUDE_MPL_SIGHANDLER
