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

#include <fstream>
#include <cstdlib>
#include "cpp_definition.h"
#include "cpp_declaration.h"
#include "cpp_emitter.h"

namespace maplefe {

bool CppEmitter::EmitCxxFiles() {
  unsigned size = mASTHandler->mASTModules.GetNum();
  for (int i = 0; i < size; i++) {
    ModuleNode *m = mASTHandler->mASTModules.ValueAtIndex(i);
    { // Emit C++ header file
      CppDecl decl(m);
      std::string decl_code = decl.Emit();
      std::string fn = decl.GetBaseFileName() + ".h"s;
      std::ofstream out(fn.c_str(), std::ofstream::out);
      out << decl_code;
      out.close();
      std::string cmd = "clang-format-10 -i "s + fn;
      std::system(cmd.c_str());
    }
    { // Emit C++ implementation file
      CppDef def(m);
      std::string def_code = def.Emit();
      std::string fn = def.GetBaseFileName() + ".cpp"s;
      std::ofstream out(fn.c_str(), std::ofstream::out);
      out << def_code;
      out.close();
      std::string cmd = "clang-format-10 -i "s + fn;
      std::system(cmd.c_str());
    }
  }
  return true;
}

std::string CppEmitter::EmitModuleNode(ModuleNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("// Filename: "s);
  str += node->GetFileName() + "\n"s;
  str += R"""(//#include "ts2cpp.h"
#include <iostream>

class Module_1 {
public: // all top level variables in the module
)""";
  mPhase = PK_DECL;
  for (unsigned i = 0; i < node->GetTreesNum(); ++i) {
    if (auto n = node->GetTree(i)) {
      str += EmitTreeNode(n);
    }
  }

  str += R"""(

public: // all top level functions in the module
void __init_func__() { // bind "this" to current module
)""";

  mPhase = PK_CODE;
  for (unsigned i = 0; i < node->GetTreesNum(); ++i) {
    if (auto n = node->GetTree(i)) {
      str += EmitTreeNode(n);
    }
  }

  str += R"""(}

  // export table here
};

Module_1 module_1;

// If the program starts from this module, generate the main function
int main(int argc, char **argv) {
  module_1.__init_func__(); // only call to its __init_func__()
  return 0;
}
)""";
  return str;
}


std::string CppEmitter::EmitFunctionNode(FunctionNode *node) {
  if (node == nullptr || mPhase == PK_CODE)
    return std::string();
  return Emitter::EmitFunctionNode(node);
}

std::string CppEmitter::EmitBinOperatorNode(BinOperatorNode *node) {
  if (node == nullptr || mPhase == PK_DECL)
    return std::string();
  std::string str = Emitter::EmitBinOperatorNode(node);
  Emitter::Replace(str, ">>>", ">>");
  return str;
}

std::string CppEmitter::EmitIdentifierNode(IdentifierNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(mPhase == PK_CODE) {
    str += node->GetName();
    if (auto n = node->GetInit()) {
      str += " = "s + EmitTreeNode(n);
    }
    mPrecedence = '\030';
    if (node->IsStmt())
      str += ";\n"s;
  } else if(mPhase == PK_DECL) {
    if (auto n = node->GetType()) {
      str += EmitTreeNode(n);
    }
    str += " "s + node->GetName();
  }
  return str;
}


std::string CppEmitter::EmitPrimTypeNode(PrimTypeNode *node) {
  if (node == nullptr && mPhase != PK_DECL)
    return std::string();
  switch(node->GetPrimType()) {
    case TY_Number:
      return "long "s; // Use long for number for now
      //return "double "s;
    case TY_Boolean:
      return "bool "s;
  }
  return Emitter::GetEnumTypeId(node->GetPrimType());
}


std::string CppEmitter::EmitDeclNode(DeclNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(mPhase == PK_CODE) {
    //std::string str(Emitter::GetEnumDeclProp(node->GetProp()));
    if (auto n = node->GetVar()) {
      str += " "s + EmitTreeNode(n);
    }
    if (auto n = node->GetInit()) {
      str += " = "s + EmitTreeNode(n);
    }
    mPrecedence = '\030';
    if (node->IsStmt())
      str += ";\n"s;
  } else if(mPhase == PK_DECL) {
    if (auto n = node->GetVar()) {
      str += " "s + EmitTreeNode(n) + ";"s;
    }
  }
  return str;
}

std::string CppEmitter::EmitCallNode(CallNode *node) {
  if (node == nullptr || mPhase != PK_CODE)
    return std::string();
  bool log = false;
  std::string str;
  if (auto n = node->GetMethod()) {
    auto s = EmitTreeNode(n);
    if(n->GetKind() == NK_Function)
      str += "("s + s + ")"s;
    else {
      if(s.compare(0, 11, "console.log") == 0) {
        str += "std::cout"s;
        log = true;
      } else
        str += s;
    }
  }
  if(!log)
    str += "("s;
  for (unsigned i = 0; i < node->GetArgsNum(); ++i) {
    if(log)
      str += " << "s + EmitTreeNode(node->GetArg(i));
    else {

      if (i)
        str += ", "s;
      if (auto n = node->GetArg(i))
        str += EmitTreeNode(n);
    }
  }
  if(!log)
    str += ")"s;
  else
    str += " << std::endl";
  mPrecedence = '\024';
  if (node->IsStmt())
    str += ";\n"s;
  return str;
}

} // namespace maplefe
