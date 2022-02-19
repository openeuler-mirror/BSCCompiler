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
        return GetIdentifierName(static_cast<StructNode *>(node)->GetStructId());
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

} // namespace maplefe
