/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MPL_RUNTIME_LINKER_GCTIB_H
#define MPL_RUNTIME_LINKER_GCTIB_H

#include "sizes.h"

namespace maplert {
void RefCal(const MClass &cls, std::vector<uint64_t> &refOffsets, std::vector<uint64_t> &weakOffsets,
    std::vector<uint64_t> &unownedOffsets, uint64_t &maxRefOffset);
void DumpGctib(const struct GCTibGCInfo &gctib);
bool ReGenGctib(ClassMetadata *classInfo, bool forceUpdate = true);
} // namespace maplert
#endif // MPL_RUNTIME_LINKER_GCTIB_H
