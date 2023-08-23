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
#include "chosen.h"

namespace maplert {
// Actual instances
ImmortalWrapper<BumpPointerAlloc> permAllocator("maple_alloc_perm", kPermMaxSpaceSize);
ImmortalWrapper<BumpPointerAlloc> zterpMetaAllocator("maple_alloc_zterp_meta", kZterpMaxSpaceSize);
ImmortalWrapper<MetaAllocator> metaAllocator("maple_alloc_meta", kMetaMaxSpaceSize);
ImmortalWrapper<DecoupleAllocator> decoupleAllocator("maple_alloc_decouple", kDecoupleMaxSpaceSize);
ImmortalWrapper<ZterpStaticRootAllocator> zterpStaticRootAllocator("maple_alloc_zterp_static_root",
                                                                   kZterpMaxSpaceSize);
ImmortalWrapper<TheAllocator> theAllocator;
} // namespace maplert
