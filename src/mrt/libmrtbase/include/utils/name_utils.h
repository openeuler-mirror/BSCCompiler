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
#ifndef MAPLE_RUNTIME_NAME_UTILS_H
#define MAPLE_RUNTIME_NAME_UTILS_H

#include <string>

namespace nameutils {

// the object type like "Ljava/lang/Class;" to "java.lang.Class"
std::string SlashNameToDot(const std::string &slashName);

// "java.lang.Class" to "Ljava/lang/Class"
std::string DotNameToSlash(const std::string &dotName);

} // namespace nameutils
#endif // MAPLE_RUNTIME_NAME_UTILS_H~
