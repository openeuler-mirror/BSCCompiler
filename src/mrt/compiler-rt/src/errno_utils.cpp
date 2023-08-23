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
#include "errno_utils.h"

#include <cerrno>
#include <string.h> // Not <cstring>. strerror_r is a POSIX function not in the C++ standard.

namespace maplert {
// NOTE: We use strerror_r, because strerror is not thread-safe.  The man page
// of strerror_l says it is thread-safe, but the man page contradicts with
// itself.
//
// Since we don't know whether we are using the XSI-compliant version or the GNU
// version of strerror_r, we use C++ function overloading to detect which
// strerror_r is provided.
//
// See: man strerror
using XSIStrErrorRType = int(*)(int, char*, size_t);
using GNUStrErrorRType = char*(*)(int, char*, size_t);

// The XSI-compliant version
std::string DoErrnoToString(XSIStrErrorRType theStrErrorRFunction, int errNum, char *buf, size_t bufSize) {
  std::string message;

  int result = theStrErrorRFunction(errNum, buf, bufSize);
  if (result == 0) {
    message = buf;
  } else {
    int anotherErrnum = errno;

    message = "Error while calling XSI-compliant strerror_r: ";

    switch (anotherErrnum) {
      case EINVAL:
        message += "The value of errNum is not a valid error number.";
        break;
      case ERANGE:
        message += "Insufficient storage was supplied to contain the error description string.";
        break;
      default:
        message += "Unexpected errno from strerror_r: " + std::to_string(anotherErrnum);
        break;
    }
  }

  return message;
}

// The GNU version
std::string DoErrnoToString(GNUStrErrorRType theStrErrorRFunction, int errNum, char *buf, size_t bufSize) {
  std::string message;

  char *result = theStrErrorRFunction(errNum, buf, bufSize);
  // On error, result will point to something like "Unkonwn error nnn".
  message = result;

  return message;
}

// Get the error message for an errno as a std::string.
//
// @param errNum The error code, from the errno macro.
//
// @return The error description string represented as an std::string. Handles
// strerror_r errors internally.
std::string ErrnoToString(int errNum) {
  static constexpr size_t kErrorBufferSize = 256;
  char buf[kErrorBufferSize]; // Cannot use std::string until C++17. Need to write to the buffer.

  // Because of function overloading, the C++ compiler will choose the
  // appropriate DoErrnoToString variant depending on the actual type of
  // ::strerror_r.
  std::string message = DoErrnoToString(::strerror_r, errNum, buf, kErrorBufferSize);

  return message;
}
}  // namespace maplert
