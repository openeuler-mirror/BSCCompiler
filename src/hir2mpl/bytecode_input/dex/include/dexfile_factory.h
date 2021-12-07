/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_FACTORY_H
#define HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_FACTORY_H

#include <memory>
#include "dexfile_interface.h"

namespace maple {
class DexFileFactory {
 public:
  std::unique_ptr<IDexFile> NewInstance() const;
};
} // namespace maple

#endif /* HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_FACTORY_H_ */
