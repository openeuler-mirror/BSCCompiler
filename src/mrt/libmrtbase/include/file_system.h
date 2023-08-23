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

#ifndef MAPLE_FS_H_
#define MAPLE_FS_H_

namespace maple {
namespace fs {
#if defined(__aarch64__)
  static constexpr char kSystemLibPath[] = "/system/lib64/";
  static constexpr char kLibcorePath[] = "/system/lib64/libmaplecore-all.so";

  static constexpr char kLibframeworkPath[] = "/system/lib64/libmapleframework.so";
#elif defined(__arm__)
  static constexpr char kSystemLibPath[] = "/system/lib/";
  static constexpr char kLibcorePath[] = "/system/lib/libmaplecore-all.so";
  static constexpr char kLibframeworkPath[] = "/system/lib/libmapleframework.so";
#endif
}; // namespace fs
} // end namespace maple
#endif // end MAPLE_FS_H_
