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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_PRAGMA_H
#define HIR2MPL_BC_INPUT_INCLUDE_BC_PRAGMA_H
#include <cstdint>
#include <memory>
#include <vector>
#include "mir_module.h"
#include "mir_pragma.h"
#include "mempool.h"

namespace maple {
namespace bc {
class BCAnnotationsDirectory {
 public:
  BCAnnotationsDirectory(MIRModule &moduleArg, MemPool &mpArg)
      : module(moduleArg),
        mp(mpArg) {}
  virtual ~BCAnnotationsDirectory() = default;
  std::vector<MIRPragma*> &EmitPragmas() {
    return EmitPragmasImpl();
  }

 protected:
  virtual std::vector<MIRPragma*> &EmitPragmasImpl() = 0;
  MIRModule &module;
  MemPool &mp;
  std::vector<MIRPragma*> pragmas;
};
}  // namespace bc
}  // namespace maple
#endif // HIR2MPL_BC_INPUT_INCLUDE_BC_PRAGMA_H
