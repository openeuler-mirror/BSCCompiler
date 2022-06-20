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
#ifndef MAPLEBE_INCLUDE_BE_RT_H
#define MAPLEBE_INCLUDE_BE_RT_H

#include <cstdint>
#include <string>

namespace maplebe {
/*
 * This class contains constants about the ABI of the runtime, such as symbols
 * for GC-related metadata in generated binary files.
 */
class RTSupport {
 public:
  static RTSupport &GetRTSupportInstance();
  uint64_t GetObjectAlignment() const {
    return kObjectAlignment;
  }
  int64_t GetArrayContentOffset() const {
    return kArrayContentOffset;
  }
  int64_t GetArrayLengthOffset() const {
    return kArrayLengthOffset;
  }
  uint64_t GetFieldSize() const {
    return kRefFieldSize;
  }
  uint64_t GetFieldAlign() const {
    return kRefFieldAlign;
  }

 protected:
  uint64_t kObjectAlignment;    /* Word size. Suitable for all Java types. */
  uint64_t kObjectHeaderSize;   /* java object header used by MM. */

#ifdef USE_32BIT_REF
  uint32_t kRefFieldSize;       /* reference field in java object */
  uint32_t kRefFieldAlign;
#else
  uint32_t kRefFieldSize;       /* reference field in java object */
  uint32_t kRefFieldAlign;
#endif /* USE_32BIT_REF */
  /* The array length offset is fixed since CONTENT_OFFSET is fixed to simplify code */
  int64_t kArrayLengthOffset;  /* shadow + monitor + [padding] */
  /* The array content offset is aligned to 8B to alow hosting of size-8B elements */
  int64_t kArrayContentOffset; /* fixed */
  int64_t kGcTibOffset;
  int64_t kGcTibOffsetAbs;

 private:
  static const std::string kObjectMapSectionName;
  static const std::string kGctibLabelArrayOfObject;
  static const std::string kGctibLabelJavaObject;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_BE_RT_H */