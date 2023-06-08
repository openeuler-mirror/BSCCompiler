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

#include "suffix_array.h"
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>
#include <set>

namespace {
  constexpr bool kLType = false;
  constexpr bool kSType = true;
  constexpr size_t kMinimumRegionLength = 4;
}

namespace maple {
bool SuffixArray::IsLmsCharacter(size_t pos) {
  return pos + 1 > 1 && pos != 0 && suffixType[pos] == kSType && suffixType[pos - 1] == kLType;
}

bool SuffixArray::IsSubStringEqual(size_t lhs, size_t rhs) {
  do {
    if (src[lhs] != src[rhs]) {
      return false;
    }
    ++lhs;
    ++rhs;
  } while (!IsLmsCharacter(lhs) && !IsLmsCharacter(rhs));
  return src[lhs] == src[rhs];
}

void SuffixArray::InducedSort() {
  for (size_t i = 0; i < length; ++i) {
    if (suffixArray[i] + 1 > 1 && suffixType[suffixArray[i] - 1] == kLType) {
      suffixArray[lBucket[src[suffixArray[i] - 1]]++] = suffixArray[i] - 1;
    }
  }
  for (size_t i = 1; i <= alphabetSize; ++i) {
    sBucket[i] = bucket[i] - 1;
  }
  for (auto i = length - 1; i + 1 > 0; --i) {
    if (suffixArray[i] + 1 > 1 && suffixType[suffixArray[i] - 1] == kSType) {
      suffixArray[sBucket[src[suffixArray[i] - 1]]--] = suffixArray[i] - 1;
    }
  }
}

void SuffixArray::BucketInit() {
  for (size_t i = 0; i < length; ++i) {
    ++bucket[src[i]];
  }
  for (size_t i = 1; i <= alphabetSize; ++i) {
    bucket[i] += bucket[i - 1];
    lBucket[i] = bucket[i - 1];
    sBucket[i] = bucket[i] - 1;
  }
}

void SuffixArray::ComputeSuffixType() {
  suffixType[length - 1] = kSType;
  for (auto i = length - 2; i + 1 > 0; --i) {
    if (src[i] < src[i + 1]) {
      suffixType[i] = kSType;
    } else if (src[i] > src[i + 1]) {
      suffixType[i] = kLType;
    } else {
      suffixType[i] = suffixType[i + 1];
    }
  }
}

void SuffixArray::FindLmsSubString() {
  for (size_t i = 1; i < length; ++i) {
    if (suffixType[i] == kSType && suffixType[i - 1] == kLType) {
      lmsSubStringPosition[lmsSubStringCount++] = i;
    }
  }
}

void SuffixArray::RenameLmsSubString() {
  auto lastLmsPos = kInvalidPosition;
  for (size_t i = 1; i < length; ++i) {
    if (IsLmsCharacter(suffixArray[i])) {
      if (lastLmsPos != kInvalidPosition && !IsSubStringEqual(lastLmsPos, suffixArray[i])) {
        ++lmsCharacterCount;
      }
      if (lastLmsPos != kInvalidPosition && lmsCharacterCount == lmsCharacterString[lastLmsPos]) {
        hasSameLmsSubstring = true;
      }
      lmsCharacterString[suffixArray[i]] = lmsCharacterCount;
      lastLmsPos = suffixArray[i];
    }
  }
  lmsCharacterString[length - 1] = 0;
}

void SuffixArray::ConstructSuffixArray() {
  std::vector<size_t> lmsString(lmsSubStringCount, 0);
  std::vector<size_t> lmsSuffixArray(lmsSubStringCount + 1, 0);
  size_t position = 0;
  for (size_t i = 0; i < length; ++i) {
    if (lmsCharacterString[i] != kInvalidPosition) {
      lmsString[position++] = lmsCharacterString[i];
    }
  }
  if (hasSameLmsSubstring) {
    SuffixArray subSA(lmsString, lmsSubStringCount, lmsCharacterCount);
    subSA.Run();
    lmsSuffixArray.assign(subSA.GetSuffixArray().begin(), subSA.GetSuffixArray().end());
  } else {
    for (size_t i = 0; i < lmsSubStringCount; ++i) {
      lmsSuffixArray[lmsString[i]] = i;
    }
  }
  lBucket[0] = sBucket[0] = 0;
  for (size_t i = 1; i <= alphabetSize; ++i) {
    lBucket[i] = bucket[i - 1];
    sBucket[i] = bucket[i] - 1;
  }
  std::fill(suffixArray.begin(), suffixArray.end(), kInvalidPosition);
  for (auto i = lmsSubStringCount - 1; i + 1 > 0; --i) {
    auto lmsSuffix = lmsSuffixArray[i];
    suffixArray[sBucket[src[lmsSubStringPosition[lmsSuffix]]]--] = lmsSubStringPosition[lmsSuffix];
  }
  InducedSort();
}

void SuffixArray::ComputeLcpArray() {
  for (size_t i = 0; i < suffixArray.size(); ++i) {
    rankArray[suffixArray[i]] = i;
  }
  for (size_t i = 0, k = 0; i < rankArray.size(); ++i) {
    if (rankArray[i] == 0) {
      continue;
    }
    k = k != 0 ? k - 1 : k;
    size_t j = suffixArray[rankArray[i] - 1];
    while (src[i + k] == src[j + k]) {
      ++k;
    }
    heightArray[rankArray[i]] = k;
  }
}

void SuffixArray::FindRepeatedSubStrings() {
  std::vector<SubStringOccurrences *> window;
  for (size_t i = 1; i < suffixArray.size(); ++i) {
    auto currLcpLength = heightArray[i];
    auto currStartPos = suffixArray[i];
    auto prevStartPos = suffixArray[i - 1];
    if (currLcpLength < kMinimumRegionLength) {
      window.clear();
      continue;
    }
    size_t index = 0;
    for (; index < window.size(); ++index) {
      auto *tempOccurrences = window[index];
      auto eleLength = tempOccurrences->length;
      if (eleLength > currLcpLength) {
        break;
      }
      (void)tempOccurrences->occurrences.emplace_back(std::make_pair(currStartPos, currStartPos + eleLength - 1));
    }
    if (index < window.size()) {
      window.resize(index);
    } else if (window.empty() || window.back()->length < currLcpLength) {
      auto startPoint = window.empty() ? kMinimumRegionLength - 1 : window.back()->length;
      for (auto j = startPoint; j < currLcpLength; ++j) {
        auto *newOccurrences = new SubStringOccurrences(j + 1);
        repeatedSubStrings.push_back(newOccurrences);
        (void)newOccurrences->occurrences.emplace_back(std::make_pair(currStartPos, currStartPos + j));
        (void)newOccurrences->occurrences.emplace_back(std::make_pair(prevStartPos, prevStartPos + j));
        (void)window.emplace_back(newOccurrences);
      }
    }
  }
}

void SuffixArray::Run(bool collectSubString) {
  BucketInit();
  ComputeSuffixType();
  FindLmsSubString();
  for (size_t i = 0; i < lmsSubStringCount; ++i) {
    suffixArray[sBucket[src[lmsSubStringPosition[i]]]--] = lmsSubStringPosition[i];
  }
  InducedSort();
  RenameLmsSubString();
  ConstructSuffixArray();
  if (collectSubString) {
    ComputeLcpArray();
    FindRepeatedSubStrings();
  }
}

void SuffixArray::Dump() {
  LogInfo::MapleLogger() << "suffix array: ";
  for (auto ele : suffixArray) {
    LogInfo::MapleLogger() << ele << " ";
  }
  LogInfo::MapleLogger() << "\n";

  LogInfo::MapleLogger() << "rank array: ";
  for (auto ele : rankArray) {
    LogInfo::MapleLogger() << ele << " ";
  }
  LogInfo::MapleLogger() << "\n";

  for (size_t i = 0; i < heightArray.size(); ++i) {
    LogInfo::MapleLogger() << i << ": ";
    LogInfo::MapleLogger() << "sa[i]: "<< suffixArray[i] << " ";
    LogInfo::MapleLogger() << "height[i]: "<< heightArray[i] << " ";
    for (size_t j = 0; j < heightArray[i]; ++j) {
      LogInfo::MapleLogger() << src[suffixArray[i] + j] << " ";
    }
    LogInfo::MapleLogger() << "\n";
  }
}
}
