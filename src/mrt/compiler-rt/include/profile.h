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
#ifndef MAPLE_RUNTIME_PROFILE_H
#define MAPLE_RUNTIME_PROFILE_H

#include <iostream>
#include <string>

#include "address.h"

#define RECORD_FUNC_NAME 0

namespace maplert {
void DumpRCAndGCPerformanceInfo(std::ostream &os);
void RecordMethod(uint64_t faddr, std::string &func, std::string &soname);
bool CheckMethodResolved(uint64_t faddr);
void DumpMethodUse(std::ostream &os);

void RecordStaticField(address_t *addr, const std::string name);
void DumpStaticField(std::ostream &os);
void ClearFuncProfile();
}

#endif
