/*
 * Copyright (C) [2021-2022] Futurewei Technologies, Inc. All rights reverved.
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
#include "cpp_emitter.h"
#include "helper.h"

namespace maplefe {

std::string CppEmitter::GetIdentifierName(TreeNode *node) {
  if (node == nullptr)
    return std::string();
  switch (node->GetKind()) {
    case NK_Identifier:
        return std::string(static_cast<IdentifierNode *>(node)->GetName());
    case NK_Decl:
        return GetIdentifierName(static_cast<DeclNode *>(node)->GetVar());
    case NK_Struct:
        // Named StructNode has name in StructId. Unamed StructNode is assigned
        // anonymous name by frontend and can be accessed using node mStrIdx
        // through node GetName() interface.
        if (auto n = static_cast<StructNode *>(node)->GetStructId())
          return GetIdentifierName(n);
        else
          return node->GetName(); // for anonomyous name
    case NK_Function:
        if (static_cast<FunctionNode *>(node)->GetFuncName())
          return GetIdentifierName(static_cast<FunctionNode *>(node)->GetFuncName());
        else
          return GenAnonFuncName(node);
    case NK_Class:
        return std::string(static_cast<ClassNode *>(node)->GetName());
    case NK_Interface:
        return std::string(static_cast<InterfaceNode *>(node)->GetName());
    case NK_UserType:
        return GetIdentifierName(static_cast<UserTypeNode *>(node)->GetId());
    case NK_TypeAlias:
        return GetIdentifierName(static_cast<TypeAliasNode *>(node)->GetId());
    case NK_Namespace:
        return GetIdentifierName(static_cast<NamespaceNode *>(node)->GetId());
    case NK_Module:
        return GetModuleName(static_cast<ModuleNode *>(node)->GetFilename());
    case NK_Literal:
        return AstDump::GetEnumLitData(static_cast<LiteralNode *>(node)->GetData());
    case NK_Declare:
        { auto n = static_cast<DeclareNode *>(node);
          auto num = n->GetDeclsNum();
          if (num == 1)
            return GetIdentifierName(n->GetDeclAtIndex(0));
          return "Failed: one decl is expected"s;
        }
    default:
        return "Failed to get the name of "s + AstDump::GetEnumNodeKind(node->GetKind());
  }
}

bool CppEmitter::IsInNamespace(TreeNode *node) {
  while (node) {
    if (node->IsNamespace())
      return true;
    node = node->GetParent();
  }
  return false;
}

std::string CppEmitter::GetNamespace(TreeNode *node) {
  std::string ns;
  while (node) {
    if (node->IsNamespace()) {
      TreeNode *id = static_cast<NamespaceNode *>(node)->GetId();
      if (id->IsIdentifier()) {
        std::string s = Emitter::EmitIdentifierNode(static_cast<IdentifierNode *>(id));
        ns = ns.empty() ? s : s + "::"s + ns;
      }
    }
    node = node->GetParent();
  }
  return ns;
}

std::string CppEmitter::GetQualifiedName(IdentifierNode *node) {
  std::string name;
  if (node == nullptr)
    return name;
  name = node->GetName();
  TreeNode *parent = node->GetParent();
  if (parent->IsField())
    return name;
  Module_Handler *handler = GetModuleHandler();
  TreeNode *decl = handler->FindDecl(node);
  if (decl == nullptr)
    return name;
  std::string ns = GetNamespace(decl);
  return ns.empty() ? name : ns + "::"s + name;
}

// Returns true if identifier is a class
bool CppEmitter::IsClassId(TreeNode* node) {
  if (node == nullptr || !node->IsIdentifier())
    return false;
  if (auto decl = mHandler->FindDecl(static_cast<IdentifierNode*>(node), true)) { // deep, cross module lookup
    if (decl->IsClass())
      return true;
    // TODO: handle type alias
  }
  return false;
}

// Returns true if the declared type of a var is a TS class
bool CppEmitter::IsVarTypeClass(TreeNode* var) {
  if (var == nullptr)
    return false;
  if (auto n = gTypeTable.GetTypeFromTypeIdx(var->GetTypeIdx())) {
    if (n->IsClass())
      return true;
  }
  return false;
}

void CppEmitter::InsertEscapes(std::string& str) {
  Emitter::Replace(str, "\\", "\\\\", 0);
  Emitter::Replace(str, "\"", "\\\"", 0);
}

bool CppEmitter::IsGenerator(TreeNode* node) {
  return mHandler->IsGeneratorUsed(node->GetNodeId());
}

FunctionNode* CppEmitter::GetGeneratorFunc(TreeNode* node) {
  return mHandler->GetGeneratorUsed(node->GetNodeId());
}

//
// Interface to get array type and dimension interface for an ArrayLiteral
// (should be just a wrapper to call TI interfaces GetArrayElemTypeId()
// and GetArrayDim(), but until the usage of those 2 interface can cover all
// use caes, this interface encaps any additional work to get array type info.
//
void CppEmitter::GetArrayTypeInfo(ArrayLiteralNode* node, int& numDim, std::string& type) {
  TypeId typeId = mHandler->GetArrayElemTypeId(node->GetNodeId());
  DimensionNode* dim  = mHandler->GetArrayDim(node->GetNodeId());
  if (dim)
    numDim = dim->GetDimensionsNum();
  switch(typeId) {
    case TY_Class: {
      unsigned tIdx = mHandler->GetArrayElemTypeIdx(node->GetNodeId());
      TreeNode* tp  = gTypeTable.GetTypeFromTypeIdx(tIdx);
      type = ObjectTypeStr(tp->GetName());
      break;
    }
    case TY_Int:
      type = "long";
      break;
    case TY_String:
      type = "std::string";
      break;
    case TY_Double:
      type = "double";
      break;
    case TY_None:
      type = "t2crt::JS_Val";
      break;
#if 0
    case TY_Array:
      type = "t2crt::Array<t2crt::JS_Val>*";
      break;
#endif
    case TY_Function:
    default:
      // TODO
      dim = 0;
      type = "TBD";
      break;
  }
  return;

#if 0
  if (!node->GetParent())
    return;

  switch(node->GetParent()->GetKind()) {
  case NK_Decl:
    // e.g. var arr: number[]=[1,2,3];
    //GetArrInfoByVarId(node, dim, type);
    break;
  case NK_Identifier:
    // e.g. class Foo { arr: number[]=[1,2,3]; }
    //GetArrInfoByClassFieldId(node, dim, type);
    break;
  case NK_FieldLiteral:
    // e.g. var: {arr:number[]} = { n:[1,2,3] };
    //GetArrInfoByObjLiteralClassField(node, dim, type);
    break;
  }
#endif
}

} // namespace maplefe
