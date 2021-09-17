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

#include "cpp_definition.h"

namespace maplefe {


std::string EmitCtorInstance(ClassNode *c) {
  std::string str, thisClass, ctor, proto, prototypeProto;
  ctor = "&Function_ctor";
  thisClass = c->GetName();
  if (c->GetSuperClassesNum() == 0) {
    proto = "Function_ctor.prototype";
    prototypeProto = "Object_ctor.prototype";
  } else {
    proto = c->GetSuperClass(0)->GetName() + "_ctor"s;
    prototypeProto = proto + ".prototype"s;
    proto.insert(0, "&"s, 0, std::string::npos);
  }
  str = "\n// Instantiate constructor\n"s;
  str += "Ctor_"s+thisClass +" "+ thisClass+"_ctor("s +ctor+","s+proto+","+prototypeProto+");\n"s;
  return str;
}

// Emit default constructor func def and instance
std::string EmitDefaultCtor(ClassNode *c) {
  if (c == nullptr)
    return std::string();

  std::string str, className;
  className = c->GetName();
  str = "\n"s;
  str += className + "*"s + " Ctor_"s + className + "::operator()("s + className + "* obj)"s;
  str += "{ return obj; }\n"s;
  str += EmitCtorInstance(c);

  return str;
}

std::string CppDef::EmitCppCtor(ClassNode* node) {
  std::string str, base, props;
  props = EmitClassProps(node);
  base = (node->GetSuperClassesNum() != 0)? node->GetSuperClass(0)->GetName() : "Object";
  str += node->GetName() + "::"s + node->GetName() + "(Function* ctor, Object* proto): "s + base + "(ctor, proto)"  + " {\n"s + props +"}\n"s;
  return str;
}

std::string CppDef::EmitModuleNode(ModuleNode *node) {
  if (node == nullptr)
    return std::string();
  std::string name = GetModuleName();
  std::string str("// TypeScript filename: "s + node->GetFileName() + "\n"s);
  str += "#include <iostream>\n#include \""s + GetBaseFileName() + ".h\"\n\n"s;

  // definition of default class constructors.
  for (unsigned i = 0; i < node->GetTreesNum(); ++i) {
    if (auto n = node->GetTree(i))
      if (n->GetKind() == NK_Class) {
        str += EmitCppCtor(static_cast<ClassNode*>(n));
        if (static_cast<ClassNode*>(n)->GetConstructorsNum() == 0)
          str += EmitDefaultCtor(static_cast<ClassNode*>(n));
      }
  }

  // definitions of all top-level functions
  isInit = false;
  CfgFunc *module = mHandler->GetCfgFunc();
  auto num = module->GetNestedFuncsNum();
  for(unsigned i = 0; i < num; ++i) {
    CfgFunc *func = module->GetNestedFuncAtIndex(i);
    TreeNode *node = func->GetFuncNode();
    TreeNode *parent =  node->GetParent();
    str += EmitTreeNode(node) + GetEnding(node);
  }

  str += "\n\nvoid "s + name + "::__init_func__() { // bind \"this\" to current module\n"s;
  isInit = true;
  for (unsigned i = 0; i < node->GetTreesNum(); ++i) {
    if (auto n = node->GetTree(i)) {
      if (n->GetKind() != NK_Class)
        str += "  "s + EmitTreeNode(n) + ";\n"s;
    }
  }
  str += "}\n\n"s + name + " _"s + name + R"""(;

// If the program starts from this module, generate the main function
int main(int argc, char **argv) {
)""" + "  _"s + name + R"""(.__init_func__(); // only call to its __init_func__()
  return 0;
}
)""";
  return str;
}

std::string CppDef::EmitExportNode(ExportNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  std::string target;
  if (auto n = node->GetTarget()) {
    target = EmitTreeNode(n);
    str += "// target: "s + target + "\n"s;
  }
  auto num = node->GetPairsNum();
  for (unsigned i = 0; i < node->GetPairsNum(); ++i) {
    if (auto n = node->GetPair(i))
      str += EmitXXportAsPairNode(n);
  }
  return HandleTreeNode(str, node);
}

std::string CppDef::EmitXXportAsPairNode(XXportAsPairNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (node->IsDefault()) {
    if (auto n = node->GetBefore())
      str += "{ "s + EmitTreeNode(n) + " as default }"s;
  } else if (node->IsEverything()) {
    str += " * "s;
    if (auto n = node->GetBefore())
      str += "as "s + EmitTreeNode(n);
  } else {
    if (auto n = node->GetBefore()) {
      if (n->GetKind() == NK_Identifier)
        str += "{ "s;
      str += EmitTreeNode(n);
      if (auto n = node->GetAfter())
        str += " as "s + EmitTreeNode(n);
      if (n->GetKind() == NK_Identifier)
        str += " }"s;
    }
  }
  str = "/* CppDef::EmitXXportAsPairNode \n "s + str + ";\n*/"s;;
  return HandleTreeNode(str, node);
}

inline std::string GetClassName(FunctionNode* f) {
  TreeNode* n = f->GetParent();
  if (n && n->GetKind()==NK_Class)
    return n->GetName();
  return ""s;
}

inline bool IsClassMethod(FunctionNode* f) {
  return (f && f->GetParent() && f->GetParent()->GetKind()==NK_Class);
}

std::map<TypeId, std::string>TypeIdToJSType = {
  // AST TypeId to t2crt JSType string mapping
  {TY_Object,  "TY_Object"},
  {TY_Function,"TY_Function"},
  {TY_Boolean, "TY_Bool"},
  {TY_Int,     "TY_Long"},
  {TY_String,  "TY_String"},
  {TY_Number,  "TY_Double"},
  {TY_Double,  "TY_Double"},
  {TY_Array,   "TY_Object"}
};

// Generate call to create obj prop with ptr to c++ class fld member
// example emit result for class Foo member f1, type long
// obj->AddProp("f1", ClassFld<long Foo::*>(&Foo::f1).NewProp(TY_Long));
std::string EmitAddPropWithClassFld(std::string objName,
                                  std::string className,
                                  std::string fdName,
                                  std::string fdCType,
                                  std::string fdJSType)  {
  std::string str;
  str = objName+ "->AddProp(\""s + fdName + "\", ClassFld<"s + fdCType +
        " "s + className + "::*>(&"s + className + "::"s + fdName +
        ").NewProp("s + fdJSType + "))"s;
  return str;
}

std::string CppDef::EmitClassProps(TreeNode* node) {
  std::string clsFd, addProp;
  MASSERT(node->GetKind()==NK_Class && "Not NK_Class node");
  ClassNode* c = static_cast<ClassNode*>(node);
  for (unsigned i = 0; i < c->GetFieldsNum(); ++i) {
    auto node = c->GetField(i);
    if (!node->IsIdentifier())
      // StrIndexSig, NumIndexSig, ComputedName have to be handled at run time
      continue;
    std::string fdName = node->GetName();
    std::string fdType = mCppDecl.GetTypeString(node, static_cast<IdentifierNode*>(node)->GetType());
    TypeId typeId = node->GetTypeId();
    addProp += "  "s
               + EmitAddPropWithClassFld("this", c->GetName(), fdName, fdType, TypeIdToJSType[typeId])
               + ";\n"s;
  }
  return "  // Add class fields to obj prop list\n"s + clsFd + addProp;
}

// "var" declarations in TS/JS functions are function scope.
// Duplicate decls may appear in different blocks within
// function but should all refer to the same function scope var.
//
// So we scan for JS_Var decls with dup names in a function and emit
// a decl list with unique names for insert into function definition.
//
// The var may be initialized to different values in different
// blocks which will done in the individual DeclNodes (re: var-dup.ts)
std::string CppDef::EmitFuncScopeVarDecls(FunctionNode *node) {
  std::unordered_map<std::string, DeclNode*>varDeclsInScope;
  ASTScope* s = node->GetScope();
  for (int i = 0; i < s->GetDeclNum(); i++) {
    // build list of var decls (no dups) in function scope
    TreeNode* n = s->GetDecl(i);
    if (n->GetKind() != NK_Decl)
      continue;
    DeclNode* d = static_cast<DeclNode*>(n);
    if (d->GetProp() == JS_Var) {
      // skip var decl name duplicates - same name but diff
      // type is illegal in typescript so not checked here
      std::unordered_map<std::string, DeclNode*>::iterator it;
      it = varDeclsInScope.find(d->GetVar()->GetName());
      if (it == varDeclsInScope.end()) {
        varDeclsInScope[d->GetVar()->GetName()] = d;
      }
    }
  }
  std::string str;
  for (auto const&[key, val] : varDeclsInScope) {
    // Emit decl for the var  (just type, name - init part emitted
    // when corresponding DeclNode processed
    str += mCppDecl.EmitTreeNode(val->GetVar()) + ";"s;
  }
  return str;
}

std::string CppDef::EmitFunctionNode(FunctionNode *node) {
  if (isInit || node == nullptr)
    return std::string();

  std::string str, className;
  if (node->IsConstructor()) {
    className = GetClassName(node);
    str = "\n"s;
    str += className + "*"s + " Ctor_"s + className + "::operator()("s + className + "* obj"s;
  } else {
    str = mCppDecl.GetTypeString(node->GetType(), node->GetType());
    if(node->GetStrIdx())
      str += " "s + (IsClassMethod(node)?GetClassName(node):GetModuleName()) + "::"s + node->GetName();
    str += "("s;
  }

  for (unsigned i = 0; i < node->GetParamsNum(); ++i) {
    if (i || node->IsConstructor())
      str += ", "s;
    if (auto n = node->GetParam(i)) {
      str += mCppDecl.EmitTreeNode(n);
    }
  }
  str += ")"s;
  int bodyPos = str.size();
  if (auto n = node->GetBody()) {
    auto varDecls = EmitFuncScopeVarDecls(node);
    auto s = EmitBlockNode(n);
    if(s.empty() || s.front() != '{')
      str += "{"s + s + "}\n"s;
    else
      str += s;
    str.insert(bodyPos+1, varDecls);
  } else
    str += "{}\n"s;

  if (node->IsConstructor()) {
    Emitter::Replace(str, "this->", "obj->", 0);
    std::string ctorBody;
    ctorBody += "  return obj;\n"s;
    str.insert(str.size()-2, ctorBody, 0, std::string::npos);
    str += EmitCtorInstance(static_cast<ClassNode*>(node->GetParent()));
  }

  return str;
}

std::string CppDef::EmitIdentifierNode(IdentifierNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  str += node->GetName();
  if (auto n = node->GetInit()) {
    str += " = "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  return str;
}

// Generate vector of ObjectProp to be passed to Object constructor.
std::string CppDef::EmitStructLiteralNode(StructLiteralNode* node) {
  std::string str;
  int stops = 2;
  str += "\n"s + ident(stops) + "std::vector<ObjectProp>({\n"s;
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (i)
      str += ",\n"s;
    if (auto field = node->GetField(i)) {
      auto lit = field->GetLiteral();
      std::string fieldName = EmitTreeNode(field->GetFieldName());
      TypeId typId = lit->GetTypeId();
      std::string fieldVal = EmitTreeNode(lit);
      str += ident(stops+1);
      switch(typId) {
        case TY_Object:
          break;
        case TY_Function:
          break;
        case TY_Array:
          //str += "std::make_pair(\""s + fieldName + "\", JS_Val(Object*("s + fieldVal + ")))"s;
          break;
        case TY_Boolean:
          str += "std::make_pair(\""s + fieldName + "\", JS_Val(bool("s + fieldVal + ")))"s;
          break;
        case TY_None:
          if (fieldVal.compare("true") == 0 || fieldVal.compare("false") == 0)
            str += "std::make_pair(\""s + fieldName + "\", JS_Val(bool("s + fieldVal + ")))"s;
          break;
        case TY_Int:
          str += "std::make_pair(\""s + fieldName + "\", JS_Val(int64_t("s + fieldVal + ")))"s;
          break;
        case TY_String:
          str += "std::make_pair(\""s + fieldName + "\", JS_Val(new std::string("s + fieldVal + ")))"s;
          break;
        case TY_Number:
        case TY_Double:
          str += "std::make_pair(\""s + fieldName + "\", JS_Val(double("s + fieldVal + ")))"s;
          break;
        case TY_Class:
          // Handle embedded ObjectLiterals recursively
          if (lit->GetKind() == NK_StructLiteral) {
            std::string props = EmitStructLiteralNode(static_cast<StructLiteralNode*>(lit));
            str += "std::make_pair(\""s + fieldName + "\", JS_Val(Object_ctor._new("s + props + ")))"s;
          }
          break;
      }
    }
  }
  str += " })"s;
  return str;
}

std::string CppDef::EmitDirectFieldInit(std::string varName, StructLiteralNode* node) {
  std::string str;
  //str += ";\n"s;
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (auto field = node->GetField(i)) {
      auto lit = field->GetLiteral();
      std::string fieldName = EmitTreeNode(field->GetFieldName());
      std::string fieldVal = EmitTreeNode(lit);
      str += ident(2) + varName + "->"s + fieldName + " = "s + fieldVal + ";\n"s;
    }
  }
  return str;
}

std::string CppDef::EmitObjPropInit(std::string varName, TreeNode* varIdType, StructLiteralNode* node) {
  if (varName.empty())
    return std::string();

  std::string str;
  // UserType can be TS interface, class, type alias, builtin (Object, Record..)
  UserTypeNode* userType = (varIdType && varIdType->IsUserType())? (UserTypeNode*)varIdType: nullptr;

  if (userType == nullptr) {
    // no type info - create instance of builtin Object with proplist
    str = varName+ " = Object_ctor._new("s + EmitTreeNode(node) + ")"s;
  } else if (mHandler->FindDecl(static_cast<IdentifierNode*>(userType->GetId())) &&
             mHandler->FindDecl(static_cast<IdentifierNode*>(userType->GetId()))->IsClass()) {
    // user def class type
    // - create obj instance of user defined class and do direct field access init
    // - todo: handle class with generics
    // - todo: generate addprop instead of direct field access if prop is not a field in class decl
    str = varName+ " = "s +userType->GetId()->GetName()+ "_ctor._new();\n"s;
    str += EmitDirectFieldInit(varName, node);
  } else {
    // type is builtin (e.g. Record) and StructNode types (e.g. TSInterface)
    // create instance of type but set constructor to the builtin Object.
    str = varName+ " = new "s +EmitUserTypeNode(userType)+ "(&Object_ctor, Object_ctor.prototype, "s + EmitTreeNode(node) +");\n"s;
    auto n = mHandler->FindDecl(static_cast<IdentifierNode*>(userType->GetId()));
    if (n && n->IsStruct() && static_cast<StructNode*>(n)->GetProp() == SProp_TSInterface) {
      str += EmitDirectFieldInit(varName, node); // do direct field init
    }
  }
  return str;
}

std::string CppDef::EmitDeclNode(DeclNode *node) {
  if (node == nullptr)
    return std::string();

  // Skip - decl and init of array objs generated by cpp_decl already
  if (node->GetParent() && node->GetParent()->IsModule() && node->GetTypeId() == TY_Array)
    return std::string();

  std::string str, varStr;
  TreeNode* idType = nullptr;
  //std::string str(Emitter::GetEnumDeclProp(node->GetProp()));

  // Global vars are already declared in .h. Function vars of type JS_Var are
  // already declared in EmitFuncSCopeVarDecls, so for both cases, emit var name.
  // For function var of JS_Let/JS_Const, emit both var type & name
  if (auto n = node->GetVar()) {
    if (isInit || node->GetProp() == JS_Var) {
      varStr = EmitTreeNode(n);            // emit var name only
    } else {
      varStr = mCppDecl.EmitTreeNode(n);   // emit both var type and name
    }
    if (n->IsIdentifier()) {
      idType = static_cast<IdentifierNode*>(n)->GetType();
    }
  }
  if (auto n = node->GetInit()) {
    if (n->IsStructLiteral())
      str += EmitObjPropInit(varStr, idType, static_cast<StructLiteralNode*>(n));
    else if (node->GetVar()->IsIdentifier() && n->IsIdentifier() && n->GetTypeId() == TY_Class)
      str += varStr + "= &"s + n->GetName() + "_ctor"s;           // init with ctor address
    else
      str += varStr + " = "s + EmitTreeNode(n);
  } else {
    str = varStr;
  }
  return str;
}

static bool QuoteStringLiteral(std::string &s, bool quoted = false) {
  if(!quoted && (s.front() != '"' || s.front() != '\''|| s.back() != '"' || s.back() != '\'' ))
    return false;
  if(!quoted)
    s = s.substr(1, s.length() - 2);
  Emitter::Replace(s, "\"", "\\\"", 0);
  s = "\"" + s + "\"s";
  return true;
}

std::string EmitSuperCtorCall(TreeNode* node) {
  while (node->GetKind() && node->GetKind() != NK_Class)
    node = node->GetParent();
  if (node && node->GetKind()==NK_Class) {
    std::string base, str;
    base = (static_cast<ClassNode*>(node)->GetSuperClassesNum() != 0)? static_cast<ClassNode*>(node)->GetSuperClass(0)->GetName() : "Object";
    str = "  "s + base + "_ctor"s;
    return str;
  }
  return ""s;
}

std::string CppDef::EmitCallNode(CallNode *node) {
  if (node == nullptr)
    return std::string();
  bool log = false;
  bool isSuper = false;
  std::string str;
  if (auto n = node->GetMethod()) {
    auto s = EmitTreeNode(n);
    if(n->GetKind() == NK_Function)
      str += "("s + s + ")"s;
    else {
      if(s.compare(0, 11, "console.log") == 0) {
        str += "std::cout"s;
        log = true;
      } else if (s.compare("super") == 0) {
        isSuper = true;
        str += EmitSuperCtorCall(node);
      }
      else {
        Emitter::Replace(s, ".", "->", 0);
        str += s;
      }
    }
  }
  if(!log)
    str += isSuper? "(obj"s : "("s;
  unsigned num = node->GetArgsNum();
  for (unsigned i = 0; i < num; ++i) {
    if(log) {
      std::string s = EmitTreeNode(node->GetArg(i));
      if(QuoteStringLiteral(s)) {
        if(num > 1)
          s += "\"'"s + s + "'\""s;
      } else if(mPrecedence <= 13) // '\015'
        s = "("s + s + ")"s;
      if (i)
        str += " << ' ' "s;
      str += " << "s + s;
    } else {
      if (i || isSuper)
        str += ", "s;
      if (auto n = node->GetArg(i))
        str += EmitTreeNode(n);
    }
  }
  if(!log)
    str += ")"s;
  else
    str += " << std::endl;";
  mPrecedence = '\024';
  return str;
}

std::string CppDef::EmitPrimTypeNode(PrimTypeNode *node) {
  return mCppDecl.EmitPrimTypeNode(node);
}

std::string CppDef::EmitPrimArrayTypeNode(PrimArrayTypeNode *node) {
  return std::string();
}

std::string CppDef::EmitArrayElementNode(ArrayElementNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetArray()) {
    str = "(*"s;
    str += EmitTreeNode(n);
    str += ")"s;
    if(mPrecedence < '\024')
      str = "("s + str + ")"s;
  }


  for (unsigned i = 0; i < node->GetExprsNum(); ++i) {
    if (auto n = node->GetExprAtIndex(i)) {
      std::string expr;
      expr = "["s + EmitTreeNode(n) + "]"s;
      if (i < node->GetExprsNum()-1)
        str  = "(*"s + str + expr  + ")"s;
      else
        str += expr;
    }
  }
  Replace(str, "?[", "?.[");

  mPrecedence = '\030';
  return HandleTreeNode(str, node);
}

std::string CppDef::EmitArrayLiteralNode(ArrayLiteralNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("{"s);
  for (unsigned i = 0; i < node->GetLiteralsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetLiteral(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += "}"s;
  return str;
}

std::string CppDef::EmitFieldNode(FieldNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetUpper()) {
    str += EmitTreeNode(n);
  }
  if (auto n = node->GetField()) {
    std::string field = EmitIdentifierNode(n);
    Emitter::Replace(field, "length", "size()");
    if (str.compare("console") == 0)
      str += "."s + field;
    else
      str += "->"s + field;
  }
  return str;
}

std::string CppDef::EmitCondBranchNode(CondBranchNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("if("s);
  if (auto n = node->GetCond()) {
    auto cond = EmitTreeNode(n);
    str += Clean(cond);
  }
  str += ")"s;
  if (auto n = node->GetTrueBranch()) {
    str += EmitTreeNode(n) + GetEnding(n);
  }
  if (auto n = node->GetFalseBranch()) {
    str += "else"s + EmitTreeNode(n) + GetEnding(n);
  }
  if(auto n = node->GetLabel()) {
    str += "__label_break_"s + EmitTreeNode(n) + ":;\n"s;
  }
  return str;
}

std::string CppDef::EmitBlockNode(BlockNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("{\n");
  for (unsigned i = 0; i < node->GetChildrenNum(); ++i) {
    if (auto n = node->GetChildAtIndex(i)) {
      str += "  "s + EmitTreeNode(n) + GetEnding(n);
    }
  }
  str += "}\n"s;
  if(auto n = node->GetLabel()) {
    str += "__label_break_"s + EmitTreeNode(n) + ":;\n"s;
  }
  mPrecedence = '\030';
  return str;
}

std::string CppDef::EmitForLoopNode(ForLoopNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  str += "for("s;
  switch(node->GetProp()) {
    case FLP_Regular:
      {
        for (unsigned i = 0; i < node->GetInitsNum(); ++i)
          if (auto n = node->GetInitAtIndex(i)) {
            if (i)
              str += ", "s;
            str += EmitTreeNode(n);
          }
        str += "; "s;
        if (auto n = node->GetCond()) {
          str += EmitTreeNode(n);
        }
        str += "; "s;
        for (unsigned i = 0; i < node->GetUpdatesNum(); ++i)
          if (auto n = node->GetUpdateAtIndex(i)) {
            if (i)
              str += ", "s;
            str += EmitTreeNode(n);
          }
        break;
      }
    case FLP_JSIn:
      {
        if (auto n = node->GetVariable()) {
          str += EmitTreeNode(n);
        }
        str += " in "s;
        if (auto n = node->GetSet()) {
          str += EmitTreeNode(n);
        }
        break;
      }
    case FLP_JSOf:
      {
        if (auto n = node->GetVariable()) {
          std::string s = EmitTreeNode(n);
          str += "auto "s + Clean(s);
        }
        str += " : "s;
        if (auto n = node->GetSet()) {
          str += EmitTreeNode(n);
          if (n->GetKind() == NK_Identifier && static_cast<IdentifierNode*>(n)->GetTypeId() == TY_Array) {
            str += "->elements"s;
          }
        }
        break;
      }
    case FLP_NA:
      return "FLP_NA"s;
    default:
      MASSERT(0 && "Unexpected enumerator");
  }
  str += ")"s;

  auto label = node->GetLabel();
  std::string lstr;
  if(label) {
    lstr = EmitTreeNode(label);
    str += "{\n"s;
  }
  if (auto n = node->GetBody()) {
    str += EmitTreeNode(n) + GetEnding(n);
  }
  if(label)
    str += "__label_cont_"s + lstr + ":;\n}\n"s + "__label_break_"s + lstr + ":;\n"s;
  return str;
}

std::string CppDef::EmitBreakNode(BreakNode *node) {
  if (node == nullptr)
    return std::string();
  auto target = node->GetTarget();
  std::string str = target ? "goto __label_break_"s + EmitTreeNode(target) : "break"s;
  return str;
}

std::string CppDef::EmitContinueNode(ContinueNode *node) {
  if (node == nullptr)
    return std::string();
  auto target = node->GetTarget();
  std::string str = target ? "goto __label_cont_"s + EmitTreeNode(target) : "continue"s;
  return str;
}


inline bool IsObjPropBracketNotation(TreeNode *node) {
  return node->GetKind() == NK_ArrayElement &&
         static_cast<ArrayElementNode*>(node)->GetArray()->GetTypeId() == TY_Class;
}

TypeId CppDef::GetTypeFromDecl(IdentifierNode* id) {
  TypeId type = TY_None;
  DeclNode* decl = static_cast<DeclNode*>(mHandler->FindDecl(id));
  if (decl && decl->GetVar() && decl->GetVar()->GetKind() == NK_Identifier) {
    IdentifierNode* var = static_cast<IdentifierNode*>(decl->GetVar());
    if (var->GetType() && var->GetType()->GetKind() == NK_PrimType) {
      type = static_cast<PrimTypeNode*>(var->GetType())->GetPrimType();
    }
  }
  return type;
}

std::string CppDef::EmitBinOperatorNode(BinOperatorNode *node) {
  if (node == nullptr)
    return std::string();
  const char *op = Emitter::GetEnumOprId(node->GetOprId());
  const Precedence precd = *op & 0x3f;
  const bool rl_assoc = *op >> 6; // false: left-to-right, true: right-to-left
  std::string lhs, rhs;
  std::string objName, propKey, propVal;
  TypeId propKeyType, propValType;
  bool islhsObjProp = false;
  if (auto n = node->GetOpndA()) {
    if ((islhsObjProp = IsObjPropBracketNotation(n)) == true) {
      // lhs is object prop with bracket notation
      ArrayElementNode *ae = static_cast<ArrayElementNode *>(n);
      MASSERT(ae->GetExprsNum() == 1 && "ArrayElementNode ExprsNum 1 expected");
      objName     = ae->GetArray()->GetName();
      propValType = node->GetTypeId();
      propKeyType = ae->GetExprAtIndex(0)->GetTypeId();
      if (propKeyType == TY_None) {
        if (ae->GetExprAtIndex(0)->GetKind() == NK_Identifier) {
          propKeyType = GetTypeFromDecl(static_cast<IdentifierNode*>(ae->GetExprAtIndex(0)));
        }
      }
      switch (propKeyType) {
        case TY_Int:
          propKey = "to_string("s + EmitTreeNode(ae->GetExprAtIndex(0)) + ")"s;
          break;
        case TY_String:
          propKey = EmitTreeNode(ae->GetExprAtIndex(0));
          break;
        case TY_Symbol:
          propKey = "to_string("s + EmitTreeNode(ae->GetExprAtIndex(0)) + ")"s;
          break;
        default:
          MASSERT(0 && "Encounter unsupported prop key type in bracket notation");
          break;
      }
    } else {
      lhs = EmitTreeNode(n);
      if (n->GetKind() == NK_Identifier && n->GetTypeId() == TY_Array)
        lhs = "*"s + lhs;
      if(precd > mPrecedence || (precd == mPrecedence && rl_assoc))
        lhs = "("s + lhs + ")"s;
    }
  }
  else
    lhs = "(NIL) "s;
  if (auto n = node->GetOpndB()) {
    rhs = EmitTreeNode(n);
    if(precd > mPrecedence || (precd == mPrecedence && !rl_assoc))
      rhs = "("s + rhs + ")"s;
  }
  else
    rhs = " (NIL)"s;
  OprId k = node->GetOprId();
  std::string str;
  if(k == OPR_Exp) {
    str = "std::pow("s + lhs + ", "s + rhs + ")";
  } else {
    switch(k) {
      case OPR_Band:
      case OPR_Bor:
      case OPR_Bxor:
      case OPR_Shl:
      case OPR_Shr:
        lhs = "static_cast<int32_t>("s + lhs + ")"s;
        break;
      case OPR_Zext:
        lhs = "static_cast<uint32_t>("s + lhs + ")"s;
        op = "\015>>";
        break;
    }
    if (islhsObjProp) {
      str = "JS_Val prop("s + rhs + ");\n"s;
      str += objName + "->AddProp(" + propKey + ", prop)"s;
    } else
      str = lhs + " "s + std::string(op + 1) + " "s + rhs;
  }
  mPrecedence = precd;
  return str;
}

std::string CppDef::EmitUnaOperatorNode(UnaOperatorNode *node) {
  if (node == nullptr)
    return std::string();
  bool isPost = node->IsPost();
  const char *op = Emitter::GetEnumOprId(node->GetOprId());
  const Precedence precd = *op & 0x3f;
  const bool rl_assoc = *op >> 6; // false: left-to-right, true: right-to-left
  std::string opr;
  if (auto n = node->GetOpnd()) {
    opr = EmitTreeNode(n);
    if(precd > mPrecedence || (precd == mPrecedence && (rl_assoc && isPost || !rl_assoc && !isPost)))
      opr = "("s + opr + ")"s;
  }
  else
      opr = "(NIL)"s;
  if(node->GetOprId() == OPR_Bcomp)
    opr = "static_cast<int32_t>("s + opr + ")"s;
  std::string str;
  if(node->IsPost())
    str = opr + std::string(op + 1) + " "s;
  else
    str = " "s + std::string(op + 1) + opr;
  mPrecedence = precd;
  return str;
}

std::string CppDef::EmitTemplateLiteralNode(TemplateLiteralNode *node) {
  if (node == nullptr)
    return std::string();
  auto num = node->GetTreesNum();
  std::string str;
  for (unsigned i = 0; i < num; ++i) {
    if (auto n = node->GetTreeAtIndex(i)) {
      if (!std::empty(str))
        str += " + "s;
      std::string s(EmitTreeNode(n));
      if(i & 0x1)
        str += "t2crt::to_string("s + s+ ")"s;
      else {
        QuoteStringLiteral(s, true);
        str += s;
      }
    }
  }
  mPrecedence = '\016';
  return str;
}

std::string CppDef::EmitLiteralNode(LiteralNode *node) {
  if (node == nullptr)
    return std::string();
  return Emitter::EmitLiteralNode(node);
}

std::string CppDef::EmitSwitchNode(SwitchNode *node) {
  if (node == nullptr)
    return std::string();
  bool doable = true;
  for (unsigned i = 0; i < node->GetCasesNum(); ++i)
    if (SwitchCaseNode* c = node->GetCaseAtIndex(i))
      for (unsigned j = 0; j < c->GetLabelsNum(); ++j) {
        auto l = c->GetLabelAtIndex(j);
        if (l && l->GetKind() == NK_SwitchLabel) {
          auto ln = static_cast<SwitchLabelNode*>(l);
          if (auto v = ln->GetValue())
            if(v->GetKind() != NK_Literal || v->GetTypeId() != TY_Int) {
              doable = false;
              goto out_of_loops;
            }
        }
      }
out_of_loops:
  std::string label;
  TreeNode* lab = node->GetLabel();
  if(lab)
    label = "__label_break_"s + EmitTreeNode(lab);
  else
    label = "__label_switch_" + std::to_string(node->GetNodeId());
  std::string str;
  if(doable) {
    str = "switch("s;
    if (TreeNode* n = node->GetExpr()) {
      std::string expr = EmitTreeNode(n);
      str += Clean(expr);
    }
    str += "){\n"s;
    for (unsigned i = 0; i < node->GetCasesNum(); ++i) {
      if(SwitchCaseNode* n = node->GetCaseAtIndex(i))
        str += EmitTreeNode(n);
    }
    str += "}\n"s;
  } else {
    std::string tmp = "__tmp_"s + std::to_string(node->GetNodeId());
    str = "do { // switch\nauto "s + tmp + " = "s;
    if (TreeNode* n = node->GetExpr()) {
      std::string expr = EmitTreeNode(n);
      str += Clean(expr);
    }
    str += ";\n"s;
    std::string body;
    std::string other = "goto "s + label + ";\n"s;;
    for (unsigned i = 0; i < node->GetCasesNum(); ++i)
      if (SwitchCaseNode* cn = node->GetCaseAtIndex(i)) {
        for (unsigned j = 0; j < cn->GetLabelsNum(); ++j)
          if (SwitchLabelNode* ln = cn->GetLabelAtIndex(j)) {
            if(ln->IsDefault())
              other = "goto __case_"s + std::to_string(cn->GetNodeId()) + ";\n"s;
            else {
              std::string le = EmitTreeNode(ln->GetValue());
              str += "if("s + tmp + " == ("s + Clean(le)
                + "))\ngoto __case_"s + std::to_string(cn->GetNodeId()) + ";\n"s;
            }
          }
        body += "__case_"s + std::to_string(cn->GetNodeId()) + ":\n"s;
        for (unsigned s = 0; s < cn->GetStmtsNum(); ++s)
          if (TreeNode* t = cn->GetStmtAtIndex(s))
            body += EmitTreeNode(t) + ";\n"s;
      }
    str += other + body;
    str += "} while(0);\n"s;
  }
  if(!doable || lab)
    str += label + ":;\n"s;
  return str;
}

std::string CppDef::EmitTypeOfNode(TypeOfNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("t2crt::__js_typeof("s), rhs;
  if (auto n = node->GetExpr())
    rhs = EmitTreeNode(n);
  str += rhs + ")"s;
  return HandleTreeNode(str, node);
}

std::string CppDef::EmitNewNode(NewNode *node) {
  if (node == nullptr)
    return std::string();

  std::string str;
  MASSERT(node->GetId() && "No mId on NewNode");
  if (node->GetId() && node->GetId()->GetTypeId() == TY_Class) {
    // Generate code to create new obj and call constructor
    str = node->GetId()->GetName() + "_ctor("s + node->GetId()->GetName() + "_ctor._new()"s;
  } else {
    str = "new "s + EmitTreeNode(node->GetId());
    str += "("s;
  }

  auto num = node->GetArgsNum();
  for (unsigned i = 0; i < num; ++i) {
    if (i || node->GetId()->GetTypeId()==TY_Class)
      str += ", "s;
    if (auto n = node->GetArg(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += ")"s;
  if (auto n = node->GetBody()) {
    str += " "s + EmitBlockNode(n);
  }
  mPrecedence = '\024';
  return HandleTreeNode(str, node);
}

static std::string MethodString(std::string &func) {
  size_t s = func.substr(0, 9) == "function " ? 9 : 0;
  return func.back() == '}' ? func.substr(s) + "\n"s : func.substr(s) + ";\n"s;
}

std::string CppDef::EmitStructNode(StructNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  const char *suffix = ";\n";
  switch(node->GetProp()) {
    case SProp_CStruct:
      str = "struct "s;
      break;
    case SProp_TSInterface:
      return std::string(); // decl already generation by CppDecl
    case SProp_TSEnum: {
      // Create the enum type object
      std::string enumClsName;
      if (auto n = node->GetStructId()) {
        enumClsName = "Enum_"s + n->GetName();
        str += enumClsName + "* "s + n->GetName() + " = new "s + enumClsName + "();\n"s;
        return str;
      }
      break;
    }
    case SProp_NA:
      str = ""s;
      return str; // todo: handle anonymous struct created for untyped object literals.
      break;
    default:
      MASSERT(0 && "Unexpected enumerator");
  }

  if (auto n = node->GetStructId()) {
    str += EmitIdentifierNode(n);
  }

  auto num = node->GetTypeParametersNum();
  if(num) {
    str += "<"s;
    for (unsigned i = 0; i < num; ++i) {
      if (i)
        str += ", "s;
      if (auto n = node->GetTypeParameterAtIndex(i))
        str += EmitTreeNode(n);
    }
    str += ">"s;
  }

  for (unsigned i = 0; i < node->GetSupersNum(); ++i) {
    str += i ? ", "s : " extends "s;
    if (auto n = node->GetSuper(i))
      str += EmitTreeNode(n);
  }
  str += " {\n"s;
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (auto n = node->GetField(i)) {
      str += EmitTreeNode(n) + suffix;
    }
  }

  if (auto n = node->GetNumIndexSig()) {
    str += EmitNumIndexSigNode(n) + "\n"s;;
  }
  if (auto n = node->GetStrIndexSig()) {
    str += EmitStrIndexSigNode(n) + "\n";
  }

  for (unsigned i = 0; i < node->GetMethodsNum(); ++i) {
    if (auto n = node->GetMethod(i)) {
      std::string func = EmitFunctionNode(n);
      func = Clean(func);
      str += MethodString(func);
    }
  }

  str += "}\n"s;
  return HandleTreeNode(str, node);
}

} // namespace maplefe
