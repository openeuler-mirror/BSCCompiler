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
#ifndef JAVA_MRT_UTIL_H_
#define JAVA_MRT_UTIL_H_
#include <string>

// provide "secure" version of common utility functions
#include "securec.h"

#include "jni.h"

#ifdef __cplusplus
namespace maplert {
std::string GetClassNametoDescriptor(const std::string &className);
extern "C" {
#endif
#define OPENSSL_VERSION_TEXT "OpenSSL 1.0.2 (compatible; BoringSSL)"
char *MRT_FuncnameToPrototypeNames(char *funcName, int argNum, char **typeNames);
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif // __cplusplus
#endif // JAVA_MRT_UTIL_H_
