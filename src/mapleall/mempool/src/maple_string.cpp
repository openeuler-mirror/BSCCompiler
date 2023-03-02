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
#include "maple_string.h"
#include "securec.h"
namespace maple {
MapleString::MapleString(const char *str, size_t size, MemPool *currMp)
    : data(NewData(*currMp, str, size)),
      memPool(currMp),
      dataLength(size) {}

MapleString::MapleString(const char *str, MemPool *currMp)
    : MapleString(str, StrLen(str), currMp) {}

MapleString::MapleString(size_t size, MemPool *currMp)
    : MapleString(nullptr, size, currMp) {}

MapleString::MapleString(const MapleString &str, MemPool *currMp)
    : MapleString(str.data, str.dataLength, currMp) {}

MapleString::MapleString(const MapleString &str)
    : MapleString(str, str.memPool) {}

MapleString::MapleString(const std::string &str, MemPool *currMp)
    : MapleString(str.data(), str.length(), currMp) {}


void MapleString::clear() {
  data = nullptr;
  dataLength = 0;
}

size_t MapleString::find(const MapleString &str, size_t pos) const {
  return UnsafeFind(str.c_str(), pos, str.dataLength);
}

size_t MapleString::find(const char *str, size_t pos) const {
  if (str == nullptr) {
    return std::string::npos;
  }
  size_t strLen = strlen(str);
  return UnsafeFind(str, pos, strLen);
}

size_t MapleString::find_last_of(const char *str, size_t pos) const {
  if (str == nullptr) {
    return std::string::npos;
  }
  size_t strLen = strlen(str);
  if ((dataLength - pos) < strLen) {
    return std::string::npos;
  }
  for (ssize_t i = static_cast<ssize_t>(dataLength - strLen); i >= static_cast<ssize_t>(pos); --i) {
    if (data[static_cast<size_t>(i)] == str[0]) {
      size_t j = 0;
      for (; j < strLen; ++j) {
        if (data[static_cast<size_t>(i) + j] == str[j]) {
          continue;
        } else {
          break;
        }
      }
      if (j == strLen) {
        return i;
      }
    }
  }
  return std::string::npos;
}

size_t MapleString::find(const char *str, size_t pos, size_t n) const {
  if (str == nullptr) {
    return std::string::npos;
  }
  return UnsafeFind(str, pos, n);
}

size_t MapleString::find(char c, size_t pos) const {
  if (dataLength == 0 || pos >= dataLength) {
    return std::string::npos;
  }
  size_t i = pos;
  for (; i < dataLength; ++i) {
    if (data[i] == c) {
      return i;
    }
  }
  return std::string::npos;
}

MapleString MapleString::substr(size_t pos, size_t len) const {
  if (len == 0) {
    MIR_FATAL("Error: MapleString substr len is 0");
  }
  if (pos > dataLength) {
    MIR_FATAL("Error: MapleString substr pos is out of boundary");
  }
  len = (len + pos) > dataLength ? (dataLength - pos) : len;
  MapleString newStr(memPool);
  newStr.data = static_cast<char*>(newStr.memPool->Malloc((1 + len) * sizeof(char)));
  for (size_t i = 0; i < len; ++i) {
    newStr[i] = this->data[i + pos];
  }
  newStr.dataLength = len;
  newStr.data[newStr.dataLength] = '\0';
  return newStr;
}

MapleString &MapleString::insert(size_t pos, const MapleString &str) {
  if (pos > dataLength || str.dataLength == 0) {
    return *this;
  }
  data = static_cast<char*>(
      memPool->Realloc(data, (1 + dataLength) * sizeof(char), (1 + dataLength + str.dataLength) * sizeof(char)));
  CHECK_FATAL(data != nullptr, "null ptr check ");
  MapleString temp(memPool);
  if (dataLength != pos) {
    temp = this->substr(pos, dataLength - pos);
  } else {
    temp = "";
  }
  dataLength += str.dataLength;
  for (size_t i = 0; i < str.dataLength; ++i) {
    data[pos + i] = str.data[i];
  }
  if (temp == nullptr) {
    CHECK_FATAL(false, "temp null ptr check");
  }
  for (size_t j = 0; j < temp.dataLength; ++j) {
    data[pos + str.dataLength + j] = temp.data[j];
  }
  data[dataLength] = '\0';
  return *this;
}

MapleString &MapleString::insert(size_t pos, const MapleString &str, size_t subPos, size_t subLen) {
  MapleString subStr = str.substr(subPos, subLen);
  this->insert(pos, subStr);
  return *this;
}

MapleString &MapleString::insert(size_t pos, const char *s) {
  if (s == nullptr) {
    return *this;
  }
  size_t sLen = strlen(s);
  if (pos > dataLength || sLen == 0) {
    return *this;
  }
  MapleString subStr(s, memPool);
  this->insert(pos, subStr);
  return *this;
}

MapleString &MapleString::insert(size_t pos, const char *s, size_t n) {
  if (s == nullptr) {
    return *this;
  }
  size_t sLen = strlen(s);
  if (pos > dataLength || sLen == 0) {
    return *this;
  }
  n = ((n > sLen) ? sLen : n);
  MapleString subStr(s, memPool);
  subStr = subStr.substr(0, n);
  this->insert(pos, subStr);
  return *this;
}

MapleString &MapleString::insert(size_t pos, size_t n, char c) {
  if (pos > dataLength) {
    return *this;
  }
  MapleString subStr(n, memPool);
  for (size_t i = 0; i < n; ++i) {
    subStr[i] = c;
  }
  this->insert(pos, subStr);
  return *this;
}

MapleString &MapleString::push_back(const char c) {
  this->append(1, c);
  return *this;
}

MapleString &MapleString::append(const MapleString &str) {
  if (str.empty()) {
    return *this;
  }
  this->insert(dataLength, str);
  return *this;
}

MapleString &MapleString::append(const std::string &str) {
  if (str.length() <= 0) {
    return *this;
  }
  this->insert(dataLength, str.c_str());
  return *this;
}

MapleString &MapleString::append(const MapleString &str, size_t subPos, size_t subLen) {
  this->append(str.substr(subPos, subLen));
  return *this;
}

MapleString &MapleString::append(const char *s) {
  if (s == nullptr) {
    return *this;
  }
  MapleString subStr(s, memPool);
  this->append(subStr);
  return *this;
}

MapleString &MapleString::append(const char *s, size_t n) {
  if (s == nullptr) {
    return *this;
  }
  MapleString subStr(s, memPool);
  this->append(subStr, 0, n);
  return *this;
}

MapleString &MapleString::append(size_t n, char c) {
  MapleString subStr(n, memPool);
  for (size_t i = 0; i < n; ++i) {
    subStr[i] = c;
  }
  this->append(subStr);
  return *this;
}

MapleString &MapleString::assign(const MapleString &str) {
  *this = str;
  return *this;
}

MapleString &MapleString::assign(const MapleString &str, size_t subPos, size_t subLen) {
  *this = str.substr(subPos, subLen);
  return *this;
}

MapleString &MapleString::assign(const char *s) {
  *this = s;
  return *this;
}

MapleString &MapleString::assign(const char *s, size_t n) {
  MapleString subStr(s, memPool);
  subStr = subStr.substr(0, n);
  *this = subStr;
  return *this;
}

MapleString &MapleString::assign(size_t n, char c) {
  MapleString subStr(n, memPool);
  for (size_t i = 0; i < n; ++i) {
    subStr[i] = c;
  }
  this->assign(subStr);
  return *this;
}

// global operators
bool operator==(const MapleString &str1, const MapleString &str2) {
  if (str1.dataLength != str2.dataLength) {
    return false;
  }
  char *tmp1 = str1.data;
  char *tmp2 = str2.data;
  while (*tmp1 != 0) {
    if (*tmp1 != *tmp2) {
      return false;
    }
    ++tmp1;
    ++tmp2;
  }
  return true;
}

bool operator==(const MapleString &str1, const char *str2) {
  if (str2 == nullptr) {
    return false;  // Should we return str1.dataLength==0 ?
  }
  size_t size = strlen(str2);
  if (str1.dataLength != size) {
    return false;
  }
  char *tmp = str1.data;
  CHECK_NULL_FATAL(tmp);
  while (*tmp != 0) {
    if (*tmp != *str2) {
      return false;
    }
    ++tmp;
    ++str2;
  }
  return true;
}

bool operator==(const char *str1, const MapleString &str2) {
  size_t size = strlen(str1);
  if (str2.dataLength != size) {
    return false;
  }
  char *tmp = str2.data;
  CHECK_NULL_FATAL(tmp);
  while (*tmp != 0) {
    if (*tmp != *str1) {
      return false;
    }
    ++tmp;
    ++str1;
  }
  return true;
}

bool operator!=(const MapleString &str1, const MapleString &str2) {
  return !(str1 == str2);
}

bool operator!=(const MapleString &str1, const char *str2) {
  return !(str1 == str2);
}

bool operator!=(const char *str1, const MapleString &str2) {
  return !(str1 == str2);
}

bool operator<(const MapleString &str1, const MapleString &str2) {
  CHECK_FATAL(!str1.empty(), "empty string check");
  CHECK_FATAL(!str2.empty(), "empty string check");
  return (strcmp(str1.c_str(), str2.c_str()) < 0);
}
}  // namespace maple
