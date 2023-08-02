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
#include "error_code.h"
#include "mpl_logging.h"

namespace maple {
void PrintErrorMessage(int ret) {
  switch (ret) {
    case kErrorNoError:
    case kErrorExitHelp:
      break;
    case kErrorExit:
      ERR(kLncErr, "Error Exit!");
      break;
    case kErrorInvalidParameter:
      ERR(kLncErr, "Invalid Parameter!");
      break;
    case kErrorInitFail:
      ERR(kLncErr, "Init Fail!");
      break;
    case kErrorFileNotFound:
      ERR(kLncErr, "File Not Found!");
      break;
    case kErrorToolNotFound:
      ERR(kLncErr, "Tool Not Found!");
      break;
    case kErrorCompileFail:
      ERR(kLncErr, "Compile Fail!");
      break;
    case kErrorNotImplement:
      ERR(kLncErr, "Not Implement!");
      break;
    case kErrorUnKnownFileType:
      ERR(kLncErr, "File Format Not Recognized!");
      break;
    case kErrorCreateFile:
      ERR(kLncErr, "Fail To Create File!");
      break;
    case kErrorNeedLtoOption:
      ERR(kLncErr, "Must enter -flto option!");
      break;
    case kErrorNoOptionFile:
      ERR(kLncErr, "Fail To Find Option File Under Lto!");
      break;
    case kErrorLtoInvalidParameter:
      ERR(kLncErr, "The lto compilation options must be consistent.!");
      break;
    default:
      break;
  }
}
}  // namespace maple
