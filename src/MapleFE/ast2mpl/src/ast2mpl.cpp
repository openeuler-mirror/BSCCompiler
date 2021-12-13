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

#include <queue>
#include <sstream>
#include <fstream>
#include <iterator>

#include "ast2mpl.h"
#include "ast_handler.h"
#include "gen_astdump.h"
#include "gen_astgraph.h"
#include "gen_aststore.h"
#include "gen_astload.h"

#include "mir_function.h"
#include "ast2mpl_builder.h"

namespace maplefe {

// starting point of AST
int A2M::ProcessAST() {
  mIndexImported = GetModuleNum();

  // loop through module handlers
  for (HandlerIndex i = 0; i < GetModuleNum(); i++) {
    Module_Handler *handler = mASTHandler->GetModuleHandler(i);
    ModuleNode *module = handler->GetASTModule();

    if (mFlags & FLG_trace_1) {
      std::cout << "============= in ProcessAST ===========" << std::endl;
      std::cout << "srcLang : " << module->GetSrcLangString() << std::endl;

      for(unsigned k = 0; k < module->GetTreesNum(); k++) {
        TreeNode *tnode = module->GetTree(k);
        if (mFlags & FLG_trace_1) {
          tnode->Dump(0);
          std::cout << std::endl;
        }
      }
    }

    if (mFlags & FLG_trace_2) {
      std::cout << "============= AstGraph ===========" << std::endl;
      AstGraph graph(module);
      graph.DumpGraph("After LoadFromAstBuf()", &std::cout);
    }
  }

  // build dependency of modules
  PreprocessModules();

  // loop through module handlers in import/export dependency order
  for (auto handler: mHandlersInOrder) {
    ModuleNode *module = handler->GetASTModule();

    // basic analysis
    handler->BasicAnalysis();

    if (mFlags & FLG_trace_2) {
      std::cout << "============= After AdjustAST ===========" << std::endl;
      for(unsigned k = 0; k < module->GetTreesNum(); k++) {
        TreeNode *tnode = module->GetTree(k);
        if (mFlags & FLG_trace_1) {
          tnode->Dump(0);
          std::cout << std::endl;
        }
      }
      AstGraph graph(module);
      graph.DumpGraph("After AdjustAST()", &std::cout);
    }

    // build CFG
    handler->BuildCFG();

    if (mFlags & FLG_trace_2) {
      handler->Dump("After BuildCFG()");
    }

    // control flow analysis
    handler->ControlFlowAnalysis();

    // type inference
    handler->TypeInference();

    if (mFlags & FLG_trace_2) {
      std::cout << "============= AstGraph ===========" << std::endl;
      AstGraph graph(module);
      graph.DumpGraph("After BuildCFG()", &std::cout);
    }

    if (mFlags & FLG_trace_2) {
      std::cout << "============= AstDump ===========" << std::endl;
      AstDump astdump(module);
      astdump.Dump("After BuildCFG()", &std::cout);
    }

    // data flow analysis
    handler->DataFlowAnalysis();

    if (mFlags & FLG_trace_2) {
      handler->Dump("After DataFlowAnalysis()");
    }
  }

  // build mpl
  if (mFlags & FLG_trace_2) {
    std::cout << "============= Ast2Mpl Build ===========" << std::endl;
  }
  maplefe::Ast2MplBuilder ast2mpl_builder(mASTHandler, mFlags);
  ast2mpl_builder.Build();

  ast2mpl_builder.mMirModule->OutputAsciiMpl("", ".mpl");
  return 0;
}

}
