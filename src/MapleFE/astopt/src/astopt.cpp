/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
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

#include "astopt.h"
#include "ast_handler.h"
#include "gen_astdump.h"
#include "gen_astgraph.h"
#include "gen_aststore.h"
#include "gen_astload.h"

namespace maplefe {

// starting point of AST
void AstOpt::ProcessAST(bool trace_a2c) {
  // loop through module handlers
  unsigned size = mASTHandler->mModuleHandlers.GetNum();
  for (int i = 0; i < size; i++) {
    Module_Handler *handler = mASTHandler->mModuleHandlers.ValueAtIndex(i);
    ModuleNode *module = handler->GetASTModule();

    mTraceAstOpt = trace_a2c;
    if (mTraceAstOpt) {
      std::cout << "============= in ProcessAST ===========" << std::endl;
      std::cout << "srcLang : " << module->GetSrcLangString() << std::endl;
    }
    for(unsigned i = 0; i < module->GetTreesNum(); i++) {
      TreeNode *tnode = module->GetTree(i);
      if (mTraceAstOpt) {
        tnode->Dump(0);
        std::cout << std::endl;
      }
    }

    // adjust source code
    handler->AdjustAST();

    // build CFG
    handler->BuildCFG();

    // loop through functions in the module
    for (auto func: handler->mModuleFuncsMap[module->GetNodeId()]) {
      handler->ASTCollectAndDBRemoval(func);

      handler->BuildDFA(func);
    }

    AstStore saveAst(module);
    saveAst.StoreInAstBuf();
  }

  return;
}
}
