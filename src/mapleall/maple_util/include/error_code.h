/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_UTIL_INCLUDE_ERROR_CODE_H
#define MAPLE_UTIL_INCLUDE_ERROR_CODE_H

namespace maple {
enum ErrorCode {
  kErrorNoError,
  kErrorExit,
  kErrorExitHelp,
  kErrorInvalidParameter,
  kErrorInitFail,
  kErrorFileNotFound,
  kErrorToolNotFound,
  kErrorCompileFail,
  kErrorNotImplement,
  kErrorUnKnownFileType
};

void PrintErrorMessage(int ret);
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_ERROR_CODE_H
