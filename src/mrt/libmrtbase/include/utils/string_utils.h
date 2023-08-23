/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * c++ utils library is licensed under Mulan PSL v2.
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
#ifndef MAPLE_RUNTIME_STRING_UTILS_H
#define MAPLE_RUNTIME_STRING_UTILS_H

#include <string>
#include <vector>

namespace stringutils {

// Split a string into string tokens according to the separator, such as blank space
std::vector<std::string> Split(const std::string &source, char separator = ' ');

// Join a string vector to a single string with separator
std::string Join(const std::vector<std::string>&, char separator);

// Format c-style args according to the format string, and return the result as a string.
std::string Format(const char *format, ...);

// Append a format string into target string.
void AppendFormat(std::string &target, const char *format, ...);

void AppendFormatV(std::string &target, const char *format, va_list args);

// Check if a string prefix is a specified string
bool WithPrefix(const std::string &source, const std::string &prefix);

// Check if a string suffix is a specified string
bool WithSuffix(const std::string &source, const std::string &suffix);

// Trim a leading white space in a specified string
void TrimLeadingWhiteSpace(std::string &source, const std::string &whiteSpaces = "\t\n\r ");

// Trim a tail white space in a specified string
void TrimTailingWhiteSpace(std::string &source, const std::string &whiteSpaces = "\t\n\r ");

} // namespace stringutils
#endif // MAPLE_RUNTIME_STRING_UTILS_H
