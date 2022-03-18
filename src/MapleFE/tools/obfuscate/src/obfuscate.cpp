/*
* Copyright (C) [2022] Futurewei Technologies, Inc. All rights reverved.
* Copyright (C) [2022] Tencent. All rights reverved.
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
#include <cstdlib>

#include "obfuscate.h"
#include "ast_handler.h"
#include "gen_astdump.h"
#include "gen_astgraph.h"
#include "gen_aststore.h"
#include "gen_astload.h"
#include "cpp_definition.h"
#include "cpp_declaration.h"
#include "a2c_util.h"

namespace maplefe {

bool Obfuscate::LoadImportedModules() {
  std::queue<std::string> queue;
  for (HandlerIndex i = 0; i < GetModuleNum(); i++) {
    Module_Handler *handler = mASTHandler->GetModuleHandler(i);
    ModuleNode *module = handler->GetASTModule();
    ImportedFiles imported(module);
    imported.VisitTreeNode(module);
    for(const auto &e: imported.mFilenames)
      queue.push(e);
  }

  bool err = false;
  while(!queue.empty()) {
    std::string filename = queue.front();
    queue.pop();
    if(mASTHandler->GetHandlerIndex(filename.c_str()) == HandlerNotFound) {
      std::ifstream input(filename, std::ifstream::binary);
      if(input.fail()) {
        std::cerr << "Error: File " << filename << " not found for imported module" << std::endl;
        err = true;
        continue;
      }
      input >> std::noskipws;
      std::istream_iterator<uint8_t> s(input), e;
      maplefe::AstBuffer vec(s, e);
      maplefe::AstLoad loadAst;
      maplefe::ModuleNode *mod = loadAst.LoadFromAstBuf(vec);
      // add mod to the vector
      while(mod) {
        mASTHandler->AddModule(mod);
        ImportedFiles imported(mod);
        imported.VisitTreeNode(mod);
        for(const auto &e: imported.mFilenames)
          queue.push(e);
        mod = loadAst.Next();
      }
    }
  }
  return err;
}

// starting point of AST
int Obfuscate::ProcessAST() {
  mIndexImported = GetModuleNum();

  // load all imported modules
  if (!(mFlags & FLG_no_imported)) {
    if (LoadImportedModules()) {
      return 1;
    }
  }

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
      std::cout << "============= After BasicAnalysis ===========" << std::endl;
      for(unsigned k = 0; k < module->GetTreesNum(); k++) {
        TreeNode *tnode = module->GetTree(k);
        if (mFlags & FLG_trace_1) {
          tnode->Dump(0);
          std::cout << std::endl;
        }
      }
      AstGraph graph(module);
      graph.DumpGraph("After BasicAnalysis()", &std::cout);
    }
  }

  if (mFlags & FLG_trace_3) {
    gStringPool.Dump();
  }

  gStringPool.SetAltStrIdxMap();

  if (mFlags & FLG_trace_3) {
    gStringPool.Dump();
    gStringPool.DumpAlt();
  }

  gStringPool.SetUseAltStr(true);

  for (auto handler: mHandlersInOrder) {
    ModuleNode *module = handler->GetASTModule();

    std::cout << "============= Emitter ===========" << std::endl;
    maplefe::Emitter emitter(handler);
    std::string code = emitter.Emit("Convert AST to TypeScript code");

    // Emit to file
    std::string of_name(module->GetFilename());
    of_name += ".obf";
    std::ofstream ofs;
    ofs.open(of_name.c_str(), std::ofstream::out);
    ofs << code;
    ofs.close();
  }

  return 0;
}

} // namespace maplefe
