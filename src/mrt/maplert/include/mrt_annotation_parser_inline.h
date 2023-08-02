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
#ifndef MRT_INCLUDE_MRT_REFLECTION_ANNOTATION_INLINE_H_
#define MRT_INCLUDE_MRT_REFLECTION_ANNOTATION_INLINE_H_
#include "mrt_annotation_parser.h"
#include "mrt_profile.h"
namespace maplert {
inline AnnoParser &AnnoParser::ConstructParser(const char *str, MClass *dClass, size_t idx) {
  AnnoParser *parser = nullptr;
  if (IsIndexParser(str)) {
    parser = new AnnoIndexParser(str + annoconstant::kLabelSize, dClass, idx);
  } else {
    parser = new AnnoAsciiParser(str + annoconstant::kLabelSize, dClass, idx);
  }
  return *parser;
}

inline bool AnnoParser::IsIndexParser(const char *str) {
  int32_t flag = str[0] - annoconstant::kOldMetaLabel;
  return static_cast<bool>(flag & 1);
}

inline bool AnnoParser::HasAnnoMember(const std::string &annoStr) {
  if (annoStr.size() >= annoconstant::kLabelSize && annoStr[annoconstant::kLabelSize] > '0') {
    return true;
  }
  return false;
}

inline bool AnnoParser::IsMemberClass(const char *str, bool &isValid) {
  int32_t flag = str[0] - annoconstant::kOldMetaLabel;
  isValid = static_cast<bool>((flag >> annoconstant::kMemberPosValidOffset) & 1);
  if (!isValid) {
    return false;
  }
  return static_cast<bool>(flag & (1 << annoconstant::kIsMemberClassOffset));
}

inline std::string AnnoParser::ParseStrNotMove() {
  size_t oldIndex = annoStrIndex;
  std::string retArr = ParseStrImpl();
  annoStrIndex = oldIndex;
  return retArr;
}

inline std::string AnnoParser::ParseStrForLastStringArray() {
  if (annoStr[annoStrIndex] == annoconstant::kAnnoArrayEndDelimiter) {
    ++annoStrIndex;
  }
  return ParseStrImpl();
}

inline double AnnoParser::ParseDoubleNum(int type) {
  return ParseNumImpl<double>(type);
}

inline int64_t AnnoParser::ParseNum(int type) {
  if (annoSize == 0) {
    return 0;
  }
  return ParseNumImpl<int64_t>(type);
}
} // namespace maplert
#endif
