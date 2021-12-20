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
#ifndef MAPLE_AST_INTERFACE_H
#define MAPLE_AST_INTERFACE_H
#include <string>
#include "ast_handler.h"
#include "mir_type.h"

namespace maple {
class LibMapleAstFile {
 public:
  LibMapleAstFile() : handler(maplefe::FLG_trace) {}
  ~LibMapleAstFile() = default;

  bool Open(const std::string &fileName);

  maplefe::AST_Handler &GetASTHandler() {
    return handler;
  }

  PrimType MapPrim(maplefe::TypeId id);
  MIRType *MapPrimType(maplefe::TypeId id);
  MIRType *MapPrimType(maplefe::PrimTypeNode *ptnode);
  MIRType *MapType(maplefe::TreeNode *type);

 private:
  maplefe::AST_Handler handler;
};
} // namespace maple
#endif // MAPLE_AST_INTERFACE_H