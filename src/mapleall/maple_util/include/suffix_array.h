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
#ifndef MAPLE_UTILS_INCLUDE_SUFFIX_ARRAY_H
#define MAPLE_UTILS_INCLUDE_SUFFIX_ARRAY_H
#include <sys/types.h>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>
#include "mpl_logging.h"
namespace maple {
using SubStringPair = std::pair<size_t, size_t>;
constexpr size_t kInvalidPosition = std::numeric_limits<size_t>::max();

class SubStringOccurrences {
 public:
  friend class SuffixArray;
  explicit SubStringOccurrences(size_t length) : length(length) {}
  virtual ~SubStringOccurrences() = default;

  const std::vector<SubStringPair> &GetOccurrences() const {
    return occurrences;
  }

  const size_t GetLength() const {
    return length;
  }

 private:
  std::vector<SubStringPair> occurrences;
  size_t length;
};

class SuffixArray {
 public:
  SuffixArray(const std::vector<size_t> src, size_t length, size_t size)
      : length(length),
        alphabetSize(size),
        src(src),
        suffixType(length, false),
        lmsSubStringPosition(length, 0),
        lmsCharacterString(length, kInvalidPosition),
        suffixArray(length, kInvalidPosition),
        rankArray(length, 0),
        heightArray(length, 0),
        bucket(size + 1, 0),
        lBucket(size + 1, 0),
        sBucket(size + 1, 0) {}
  bool IsLmsCharacter(size_t pos);
  bool IsSubStringEqual(size_t lhs, size_t rhs);
  void InducedSort();
  void BucketInit();
  void ComputeSuffixType();
  void FindLmsSubString();
  void RenameLmsSubString();
  void ConstructSuffixArray();
  void ComputeLcpArray();
  void FindRepeatedSubStrings();
  void Run(bool collectSubString = false);
  void Dump();

  const std::vector<size_t> &GetSuffixArray() const {
    return suffixArray;
  }

  const std::vector<size_t> &GetHeightArray() const {
    return heightArray;
  }

  const std::vector<SubStringOccurrences *> &GetRepeatedSubStrings() const {
    return repeatedSubStrings;
  }

 private:
  size_t length = 1;
  size_t alphabetSize;
  size_t lmsSubStringCount = 0;
  size_t lmsCharacterCount = 1;
  bool hasSameLmsSubstring = false;
  const std::vector<size_t> src;
  std::vector<bool> suffixType;
  std::vector<size_t> lmsSubStringPosition;
  std::vector<size_t> lmsCharacterString;
  std::vector<size_t> suffixArray;
  std::vector<size_t> rankArray;
  std::vector<size_t> heightArray;
  std::vector<size_t> bucket;
  std::vector<size_t> lBucket;
  std::vector<size_t> sBucket;
  std::vector<SubStringOccurrences *> repeatedSubStrings;
};
}
#endif
