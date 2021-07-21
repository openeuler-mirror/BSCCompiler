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

#include "cpp_declaration.h"
#include "gen_astvisitor.h"
#include <algorithm>

namespace maplefe {

class ClassDecls : public AstVisitor {
  private:
    CppDecl     *mCppDecl;
    std::string  mDecls;

  public:
    ClassDecls(CppDecl *c) : mCppDecl(c) {}

    ClassNode *VisitClassNode(ClassNode *node) {
      mDecls += mCppDecl->EmitTreeNode(node);
      return node;
    }
    std::string GetDecls() { return mDecls; }
};

class CollectDecls : public AstVisitor {
  private:
    CppDecl     *mCppDecl;
    std::string  mDecls;

  public:
    CollectDecls(CppDecl *c) : mCppDecl(c) {}

    FunctionNode *VisitFunctionNode(FunctionNode *node) {
      return node;
    }

    LambdaNode *VisitLambdaNode(LambdaNode *node) {
      return node;
    }

    DeclNode *VisitDeclNode(DeclNode *node) {
      mDecls += mCppDecl->EmitTreeNode(node);
      return node;
    }

    std::string GetDecls() { return mDecls; }
};


std::string CppDecl::EmitModuleNode(ModuleNode *node) {
  if (node == nullptr)
    return std::string();
  std::string name = GetModuleName();
  std::string header("__");
  for(auto &c : name)
    header += std::toupper(c);
  header += "__HEADER__\n";
  std::string str("// TypeScript filename: "s + node->GetFileName() + "\n"s);
  str += "#ifndef "s + header + "#define "s + header;
  str += R"""(
#include "ts2cpp.h"
using namespace t2crt;

)""";

  // declarations of user defined classes
  ClassDecls clsDecls(this);
  clsDecls.VisitTreeNode(node);
  str += clsDecls.GetDecls();

  str += R"""(
class )""" + name + R"""( : public t2crt::Object {
public: // all top level variables in the module
)""";

  CollectDecls decls(this);
  decls.VisitTreeNode(node);
  str += decls.GetDecls();

  str += R"""(

public: // all top level functions in the module
void __init_func__();
)""";

  // declarations of all top-level functions
  CfgFunc *module = mHandler->GetCfgFunc();
  auto num = module->GetNestedFuncsNum();
  for(unsigned i = 0; i < num; ++i) {
    CfgFunc *func = module->GetNestedFuncAtIndex(i);
    TreeNode *node = func->GetFuncNode();
    if (node->GetParent() && node->GetParent()->GetKind() != NK_Class) {
      str += EmitTreeNode(node);
    }
  }

  str += R"""(
  // exports
  Object exports;
};

extern )""" + name + " _"s + name + ";\n#endif\n"s;
  return str;
}

std::string CppDecl::EmitFunctionNode(FunctionNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str(GetTypeString(node->GetType(), node->GetType()));
  if(node->GetStrIdx())
    str += " "s + node->GetName();
  str += "("s;
  for (unsigned i = 0; i < node->GetParamsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetParam(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += ");\n"s;
  return str;
}

std::string CppDecl::EmitBinOperatorNode(BinOperatorNode *node) {
  return std::string();
}

std::string CppDecl::EmitIdentifierNode(IdentifierNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str(GetTypeString(node, node->GetType()));
  str += " "s + node->GetName();
  return str;
}

std::string CppDecl::EmitPrimTypeNode(PrimTypeNode *node) {
  if (node == nullptr)
    return std::string();
  return GetTypeString(node);
}

std::string CppDecl::EmitDeclNode(DeclNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetVar()) {
    str += " "s + EmitTreeNode(n) + ";\n"s;
  }
  return str;
}

std::string CppDecl::EmitCallNode(CallNode *node) {
  return std::string();
}

std::string CppDecl::EmitCondBranchNode(CondBranchNode *node) {
  return std::string();
}

std::string CppDecl::EmitForLoopNode(ForLoopNode *node) {

  return std::string();
}

std::string CppDecl::EmitWhileLoopNode(WhileLoopNode *node) {
  return std::string();
}

std::string CppDecl::EmitDoLoopNode(DoLoopNode *node) {
  return std::string();
}

std::string CppDecl::EmitAssertNode(AssertNode *node) {
  return std::string();
}

std::string CppDecl::EmitArrayLiteralNode(ArrayLiteralNode *node) {
  return std::string();
}

std::string CppDecl::EmitPrimArrayTypeNode(PrimArrayTypeNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetPrim()) {
    str = "std::vector<"s + EmitPrimTypeNode(n) + "> "s;;
    auto d = node->GetDims();
    if(d && d->GetKind() == NK_Dimension) {
      auto num = (static_cast<DimensionNode *>(d))->GetDimensionsNum();
      for (unsigned i = 1; i < num; ++i) {
        str = "std::vector<"s + str + "> "s;;
      }
    }
  }
  /*
  if (auto n = node->GetDims()) {
    str += EmitDimensionNode(n);
  }
  */
  return str;
}

std::string CppDecl::EmitFieldNode(FieldNode *node) {
  return std::string();
}

// TODO: Add other builtin obj types
std::vector<std::string>builtins = {"Object", "Function", "Array"};

bool IsBuiltinObj(std::string name) {
 return std::find(builtins.begin(), builtins.end(), name) != builtins.end();
}

std::string CppDecl::GetTypeString(TreeNode *node, TreeNode *child) {
  std::string str;
  if (node) {
    TypeId k = node->GetTypeId();
    if (k == TY_None) {
      switch(node->GetKind()) {
        case NK_PrimType:
          k = static_cast<PrimTypeNode*>(node)->GetPrimType();
          break;
        case NK_Identifier:
        case NK_Function:
          if (child && child->GetKind() == NK_UserType) {
            // UserType - handle user defined classes and builtin obj types
            UserTypeNode* n = node->GetKind() == NK_Identifier?
              static_cast<UserTypeNode*>((static_cast<IdentifierNode *>(node))->GetType()):
              static_cast<UserTypeNode*>((static_cast<FunctionNode *>(node))->GetType());
            if (n && n->GetId() && n->GetId()->GetKind() == NK_Identifier) {
              if (n->GetId()->GetTypeId() == TY_Class)
                str = n->GetName() + "*"s;
              else if (IsBuiltinObj(n->GetId()->GetName()))
                str = "t2crt::"s + n->GetId()->GetName() + "*"s;
              return str;
            }
          }
          break;
      }
    }
    switch(k) {
      case TY_Object:
        return "t2crt::Object* "s;
      case TY_Function:
        return "t2crt::Object* "s;
      case TY_Boolean:
        return "bool "s;
      case TY_Int:
        return "long "s;
      case TY_String:
        return "std::string "s;
      case TY_Number:
      case TY_Double:
        return "double "s;
    }
    {
      str = child ? EmitTreeNode(child) : Emitter::GetEnumTypeId(k);
      if (str != "none"s)
        return str + " "s;
    }
  }
  return "t2crt::JS_Val "s;
}

std::string CppDecl::EmitUserTypeNode(UserTypeNode *node) {
  if (node == nullptr)
    return std::string();
  {
    auto k = node->GetType();
    if(k == UT_Union || k == UT_Inter)
      return "t2crt::JS_Val"s;
  }
  std::string str;
  if (auto n = node->GetId()) {
    str = EmitTreeNode(n);
    auto num = node->GetTypeGenericsNum();
    if(num) {
      str += "<"s;
      for (unsigned i = 0; i < num; ++i) {
        if (i)
          str += ", "s;
        if (auto n = node->GetTypeGeneric(i)) {
          str += EmitTreeNode(n);
        }
      }
      str += ">"s;
    }
  }

  auto k = node->GetType();
  if(k != UT_Regular) {
    if(!str.empty())
      str = "using "s + str + " = "s;
    std::string op = k == UT_Union ? " | "s : " & "s;
    for (unsigned i = 0; i < node->GetUnionInterTypesNum(); ++i) {
      if(i)
        str += op;
      str += EmitTreeNode(node->GetUnionInterType(i));
    }
  }

  if (auto n = node->GetDims()) {
    str += EmitDimensionNode(n);
  }
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string CppDecl::EmitClassNode(ClassNode *node) {
  std::string str;
  std::string base;

  if (node == nullptr)
    return std::string();

  // 1. c++ class for JS object
  base = (node->GetSuperClassesNum() != 0)? node->GetSuperClass(0)->GetName() : "Object";
  str += "class "s + node->GetName() + " : public "s + base + " {\n"s;

  str += "  public:\n";
  // constructor decl,  field init
  for (unsigned i = 0; i < node->GetConstructorsNum(); ++i) {
    if (auto t = node->GetConstructor(i)) {
      std::string init;
      for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
        if (i)
          init += ", "s;
        if (IdentifierNode* n = reinterpret_cast<IdentifierNode *>(node->GetField(i))) {
          if (n->GetInit()) {
            init += n->GetName();
            init += "("s + EmitTreeNode(n->GetInit()) + ")"s;
          }
        }
      }
      str += node->GetName() + "(Function* ctor, Object* proto): "s + base + "(ctor, proto)" + (init.empty()? init: ","s + init) + " {}\n"s;
      str += "~"s + node->GetName() + "(){}\n";
    }
  }
  // field decl. TODO: handle static, private, protected attrs.
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    str += EmitTreeNode(node->GetField(i)) + ";";
  }
  for (unsigned i = 0; i < node->GetMethodsNum(); ++i) {
    str += EmitFunctionNode(node->GetMethod(i));
  }
  str += "};\n\n";

  // 2. c++ class for JS object's corresponding JS constructor
  base = (node->GetSuperClassesNum() != 0)? ("Ctor_"s+node->GetSuperClass(0)->GetName()) : "Function";
  str += "class "s + "Ctor_" + node->GetName() + " : public "s + base + " {\n"s;
  str += "  public:\n";
  str += "Ctor_"s+node->GetName()+"(Function* ctor, Object* proto, Object* prototype_proto) : "+base+"(ctor, proto, prototype_proto) {}\n";

  for (unsigned i = 0; i < node->GetConstructorsNum(); ++i) {
    std::string ctor;
    if (auto c = node->GetConstructor(i)) {
      ctor = "  "s+node->GetName() + "* operator()("s;
      for (unsigned k = 0; k < c->GetParamsNum(); ++k) {
        if (k)
          ctor += ", "s;
        if (auto n = c->GetParam(k)) {
          ctor += EmitTreeNode(n);
        }
      }
      ctor += ");";
      str += ctor;
    }
  }
  str += "};\n\n";
  return str;
}


} // namespace maplefe
