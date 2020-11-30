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
#ifndef MAPLE_RUNTIME_DEFS_H
#define MAPLE_RUNTIME_DEFS_H

#include <type_traits>

namespace maplert {
// utility class to avoid un-ordered static global destruction
template<class T>
class ImmortalWrapper {
 public:
  using pointer = typename std::add_pointer<T>::type;
  using lref = typename std::add_lvalue_reference<T>::type;

  template<class... Args>
  ImmortalWrapper(Args && ...args) {
    new(buffer) T(std::forward<Args>(args)...);
  }
  ImmortalWrapper(const ImmortalWrapper&) = delete;
  ImmortalWrapper &operator=(const ImmortalWrapper&) = delete;
  ~ImmortalWrapper() = default;
  inline pointer operator->() {
    return reinterpret_cast<pointer>(buffer);
  }

  inline lref operator*() {
    return reinterpret_cast<lref>(buffer);
  }

 private:
  alignas(T) unsigned char buffer[sizeof(T)];
};
} // namespace maplert
#endif // MAPLE_RUNTIME_DEFS_H
