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
#ifndef MRT_NAMEMANGLER_API_H_
#define MRT_NAMEMANGLER_API_H_

#include <string>
#include "mrt_api_common.h"

// The namespace is shared between RT and compiler
namespace namemanglerapi {
MRT_EXPORT std::string MangleForJni(const std::string &s);
MRT_EXPORT std::string MangleForJniDex(const std::string &s);
}

#endif //MRT_NAMEMANGLER_API_H_
