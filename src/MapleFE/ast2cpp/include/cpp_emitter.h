/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.
 *
 * OpenArkFE is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *  http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef __CPPEMITTER_HEADER__
#define __CPPEMITTER_HEADER__

#include "ast.h"
#include "ast_attr.h"
#include "ast_module.h"
#include "ast_handler.h"
#include "ast_type.h"

#include "emitter.h"

namespace maplefe {

enum Phase { PK_DECL, PK_CODE };

class CppEmitter : public Emitter {
private:
  AST_Handler *mASTHandler;
  Phase mPhase;
public:
  CppEmitter(AST_Handler *h) : mASTHandler(h), Emitter(h) {}

  std::string Emit(const char *title) {
    std::string code;
    code = "// [Beginning of CppEmitter: "s + title + "\n"s;
    unsigned size = mASTHandler->mASTModules.GetNum();
    for (int i = 0; i < size; i++) {
      ModuleNode *mod = mASTHandler->mASTModules.ValueAtIndex(i);
      code += EmitTreeNode(mod);
    }
    code += "// End of CppEmitter]\n"s;
    return code;
  }

  std::string EmitModuleNode(ModuleNode *node);
  std::string EmitFunctionNode(FunctionNode *node);
  std::string EmitBinOperatorNode(BinOperatorNode *node);
  std::string EmitDeclNode(DeclNode *node);
  std::string EmitCallNode(CallNode *node);
  std::string EmitIdentifierNode(IdentifierNode *node);
  std::string EmitPrimTypeNode(PrimTypeNode *node);
};

} // namespace maplefe
#endif
