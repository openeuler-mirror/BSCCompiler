/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef STRING_VIEW_FORMAT_H
#define STRING_VIEW_FORMAT_H
#if defined(USE_LLVM) || defined(__ANDROID__) || defined(DARWIN)
#include <string_view>
using StringView = std::string_view;
#else
#include <experimental/string_view>
using StringView = std::experimental::string_view;
#endif
#endif // STRING_VIEW_FORMAT_H
