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

#include "emitter.h"
#include <cstring>
#include <limits>
#include <cctype>

namespace maplefe {

std::string Emitter::Emit(const char *title) {
  std::string code;
  code = "// [Beginning of Emitter: "s + title + "\n"s;
  code += EmitTreeNode(mASTModule);
  code += "// End of Emitter]\n"s;
  return code;
}

std::string Emitter::Clean(std::string &s) {
  auto len = s.length();
  if(len >= 2 && s.substr(len - 2) == ";\n")
    return s.erase(len - 2);
  return s;
}

std::string Emitter::GetBaseFileName() {
  std::string str(mASTModule->GetFileName());
  auto len = str.length();
  if(len >= 3 && str.substr(len - 3) == ".ts")
    return str.erase(len - 3);
  return str;
}

std::string Emitter::GetModuleName(const char *p) {
  std::string str = p ? p : GetBaseFileName();
  size_t pos = str.rfind("/", std::string::npos);
  str = pos == std::string::npos ? str : str.substr(pos);
  for (auto &c : str)
    if(std::ispunct(c))
        c = '_';
  return "M_"s + str;
}

std::string Emitter::GetEnumAttrId(AttrId k) {
  std::string str(AstDump::GetEnumAttrId(k) + 5);
  Emitter::Replace(str, "etter", "et");
  str += " "s;
  return str;
}

void Emitter::Replace(std::string &str, const char *o, const char *n, int cnt) {
  size_t len = std::strlen(o);
  if(cnt > 0) {
    size_t index = 0;
    int num = cnt;
    do {
      index = str.find(o, index);
      if (index == std::string::npos) break;
      str.replace(index, len, n);
      index += len;
    } while(--num && index < str.size());
  } else {
    size_t index = std::string::npos;
    int num = cnt ? -cnt : std::numeric_limits<int>::max();
    do {
      index = str.rfind(o, index);
      if (index == std::string::npos) break;
      str.replace(index, len, n);
    } while(--num);
  }
}

std::string Emitter::EncodeLiteral(std::string &str) {
  std::string enc;
  for (auto&c : str) {
    switch(c) {
      case '\'': enc += "\\'"s; break;
      case '\"': enc += "\\\""s; break;
      case '\?': enc += "\\?"s; break;
      case '\\': enc += "\\\\"s; break;
      case '\a': enc += "\\a"s; break;
      case '\b': enc += "\\b"s; break;
      case '\f': enc += "\\f"s; break;
      case '\n': enc += "\\n"s; break;
      case '\r': enc += "\\r"s; break;
      case '\t': enc += "\\t"s; break;
      case '\v': enc += "\\v"s; break;
      default: enc += c; // TODO: Unicode
    }
  }
  return enc;
}

std::string Emitter::EmitAnnotationNode(AnnotationNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetId()) {
    str += " "s + EmitIdentifierNode(n);
  }
  if (auto n = node->GetType()) {
    str += " "s + EmitAnnotationTypeNode(n);
  }

  for (unsigned i = 0; i < node->GetArgsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetArgAtIndex(i)) {
      str += " "s + EmitTreeNode(n);
    }
  }

  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitAsTypeNode(AsTypeNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetType()) {
    str += " as "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitIdentifierNode(IdentifierNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  for (unsigned i = 0; i < node->GetAttrsNum(); ++i) {
    str += GetEnumAttrId(node->GetAttrAtIndex(i));
  }
  if(node->IsRestParam())
    str += "..."s;
  str += node->GetName();
  if(node->IsOptionalParam() || node->IsOptional())
    str += "?"s;
  //if (auto n = node->GetDims()) {
  //  str += " "s + EmitDimensionNode(n);
  //}

  if (auto n = node->GetType()) {
    str += ": "s + EmitTreeNode(n);
  }
  if (auto n = node->GetInit()) {
    str += " = "s + EmitTreeNode(n);
  }
  for (unsigned i = 0; i < node->GetAsTypesNum(); ++i) {
    if (auto n = node->GetAsTypeAtIndex(i)) {
      str += " "s + EmitAsTypeNode(n);
    }
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitFunctionNode(FunctionNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  auto body = node->GetBody();
  for (unsigned i = 0; i < node->GetAttrsNum(); ++i) {
    str += GetEnumAttrId(node->GetAttrAtIndex(i));
  }
  if(str.empty())
    str = "function "s + str;
  if(node->GetStrIdx())
    str += node->GetName();

  auto num = node->GetTypeParamsNum();
  if(num) {
    str += "<"s;
    for (unsigned i = 0; i < num; ++i) {
      if (i)
        str += ", "s;
      if (auto n = node->GetTypeParamAtIndex(i)) {
        str += EmitTreeNode(n);
      }
    }
    str += ">"s;
  }

  /*
  for (unsigned i = 0; i < node->GetAnnotationsNum(); ++i) {
    if (auto n = node->GetAnnotationAtIndex(i)) {
      str += " "s + EmitAnnotationNode(n);
    }
  }

  for (unsigned i = 0; i < node->GetThrowsNum(); ++i) {
    if (auto n = node->GetThrowAtIndex(i)) {
      str += " "s + EmitExceptionNode(n);
    }
  }
  */

  str += "("s;
  for (unsigned i = 0; i < node->GetParamsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetParam(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += ")"s;

  if (auto n = node->GetAssert())
    str += " : asserts "s + EmitTreeNode(n);

  if (auto n = node->GetType()) {
    std::string s = EmitTreeNode(n);
    if(!s.empty())
      str += (body ? " : "s : " => "s) + s;
  }

  if (body) {
    auto s = EmitBlockNode(body);
    if(s.empty() || s.front() != '{')
      str += "{"s + s + "}\n"s;
    else
      str += s;
  }
  /*
  if (auto n = node->GetDims()) {
    str += " "s + EmitDimensionNode(n);
  }
  str += " "s + std::to_string(node->IsConstructor());
  */
  mPrecedence = '\030';
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitUserTypeNode(UserTypeNode *node) {
  if (node == nullptr)
    return std::string();
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
      str = "type "s + str + " = "s;
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

std::string Emitter::EmitPackageNode(PackageNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "package";
  if (auto n = node->GetPackage()) {
    str += " "s + EmitTreeNode(n);
  }
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitXXportAsPairNode(XXportAsPairNode *node) {
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
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitDeclareNode(DeclareNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetDecl()) {
    str += "declare "s + EmitTreeNode(n);
  }
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitExportNode(ExportNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "export "s;
  auto num = node->GetPairsNum();
  for (unsigned i = 0; i < node->GetPairsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetPair(i))
      str += EmitXXportAsPairNode(n);
  }
  Replace(str, "}, {", ", ", 0);
  if (auto n = node->GetTarget()) {
    str += " from "s + EmitTreeNode(n);
  }

  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitImportNode(ImportNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "import "s;
  /*
  switch (node->GetProperty()) {
    case ImpNone:
      break;
    case ImpType:
      break;
    case ImpStatic:
      break;
    case ImpSingle:
      break;
    case ImpAll:
      break;
    case ImpLocal:
      break;
    case ImpSystem:
      break;
    default:
      MASSERT(0 && "Unexpected enumerator");
  }
  */
  auto num = node->GetPairsNum();
  for (unsigned i = 0; i < node->GetPairsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetPair(i)) {
      std::string s = EmitXXportAsPairNode(n);
      auto len = s.length();
      if(len > 13 && s.substr(len - 13) == " as default }"s)
        str += s.substr(1, len - 13); // default export from a module
      else
        str += s;
    }
  }
  if (auto n = node->GetTarget()) {
    str += " from "s + EmitTreeNode(n);
  }
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitUnaOperatorNode(UnaOperatorNode *node) {
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
  std::string str;
  if(node->IsPost())
    str = opr + std::string(op + 1) + " "s;
  else
    str = " "s + std::string(op + 1) + opr;
  mPrecedence = precd;
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitBinOperatorNode(BinOperatorNode *node) {
  if (node == nullptr)
    return std::string();
  const char *op = Emitter::GetEnumOprId(node->GetOprId());
  const Precedence precd = *op & 0x3f;
  const bool rl_assoc = *op >> 6; // false: left-to-right, true: right-to-left
  std::string lhs, rhs;
  if (auto n = node->GetOpndA()) {
    lhs = EmitTreeNode(n);
    if(precd > mPrecedence || (precd == mPrecedence && rl_assoc))
      lhs = "("s + lhs + ")"s;
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
  std::string str(lhs + " "s + std::string(op + 1) + " "s + rhs);
  mPrecedence = precd;
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitTerOperatorNode(TerOperatorNode *node) {
  if (node == nullptr)
    return std::string();
  const Precedence precd = '\004';
  const bool rl_assoc = true; // true: right-to-left
  std::string str;
  if (auto n = node->GetOpndA()) {
    str = EmitTreeNode(n);
    if(precd > mPrecedence || (precd == mPrecedence && rl_assoc))
      str = "("s + str + ")"s;
  }
  str += " ? "s;
  if (auto n = node->GetOpndB()) {
    str += EmitTreeNode(n);
  }
  str += " : "s;
  if (auto n = node->GetOpndC()) {
    auto s = EmitTreeNode(n);
    if(precd > mPrecedence)
      s = "("s + s + ")"s;
    str += s;
  }
  mPrecedence = '\004';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitTypeAliasNode(TypeAliasNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "type "s;
  if (auto n = node->GetId()) {
    str += EmitUserTypeNode(n);
  }
  if (auto n = node->GetAlias()) {
    str += " = "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return str;
}

std::string Emitter::EmitConditionalTypeNode(ConditionalTypeNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetTypeA()) {
    str = EmitTreeNode(n);
  }
  if (auto n = node->GetTypeB()) {
    str += " extends "s + EmitTreeNode(n);
  }
  if (auto n = node->GetTypeC()) {
    str += " ? "s + EmitTreeNode(n);
  }
  if (auto n = node->GetTypeD()) {
    str += " : "s + EmitTreeNode(n);
  }
  if (node->IsStmt())
    str += ";\n"s;
  return str;
}

std::string Emitter::EmitTypeParameterNode(TypeParameterNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetId()) {
    str = EmitTreeNode(n);
  }
  if (auto n = node->GetDefault()) {
    str += " = "s + EmitTreeNode(n);
  }
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitBlockNode(BlockNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(auto n = node->GetLabel()) {
    str = EmitTreeNode(n) + ":\n"s;
  }
  str += "{\n"s;
  for (unsigned i = 0; i < node->GetChildrenNum(); ++i) {
    if (auto n = node->GetChildAtIndex(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += "}\n"s;

  /*
  str += " "s + std::to_string(node->IsInstInit());

  for (unsigned i = 0; i < node->GetAttrsNum(); ++i) {
    str += " "s + AstDump::GetEnumAttrId(node->GetAttrAtIndex(i));
  }

  if (auto n = const_cast<TreeNode *>(node->GetSync())) {
    str += " "s + EmitTreeNode(n);
  }
  */
  mPrecedence = '\030';
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitNewNode(NewNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "new "s;
  if (auto n = node->GetId()) {
    str += " "s + EmitTreeNode(n);
  }
  str += "("s;
  auto num = node->GetArgsNum();
  for (unsigned i = 0; i < num; ++i) {
    if (i)
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
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitDeleteNode(DeleteNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("delete "s);
  if (auto n = node->GetExpr()) {
    str += EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitDimensionNode(DimensionNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  for (unsigned i = 0; i < node->GetDimensionsNum(); ++i) {
    auto n = node->GetDimension(i);
    std::string d(n ? std::to_string(n) : ""s);
    str += "["s + d + "]"s;
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitDeclNode(DeclNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str(Emitter::GetEnumDeclProp(node->GetProp()));
  if (auto n = node->GetVar()) {
    str += " "s + EmitTreeNode(n);
  }
  if (auto n = node->GetInit()) {
    str += " = "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitAnnotationTypeNode(AnnotationTypeNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetId()) {
    str += " "s + EmitIdentifierNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitCastNode(CastNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetDestType()) {
    str += "("s + EmitTreeNode(n) + ")";
  }
  if (auto n = node->GetExpr()) {
    str += EmitTreeNode(n);
  }
  mPrecedence = '\021';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitParenthesisNode(ParenthesisNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetExpr()) {
    str += "("s + EmitTreeNode(n) + ")"s;
  }
  mPrecedence = '\025';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitFieldNode(FieldNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetUpper()) {
    str += EmitTreeNode(n);
  }
  if (auto n = node->GetField()) {
    str += "."s + EmitIdentifierNode(n);
  }
  mPrecedence = '\024';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitArrayElementNode(ArrayElementNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetArray()) {
    str = EmitTreeNode(n);
  }

  for (unsigned i = 0; i < node->GetExprsNum(); ++i) {
    if (auto n = node->GetExprAtIndex(i)) {
      str += "["s + EmitTreeNode(n) + "]"s;
    }
  }
  Replace(str, "?[", "?.[");

  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitArrayLiteralNode(ArrayLiteralNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("["s);
  for (unsigned i = 0; i < node->GetLiteralsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetLiteral(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += "]"s;
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitBindingElementNode(BindingElementNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(node->IsRest())
    str += "..."s;
  if (auto n = node->GetVariable()) {
    str += " "s + EmitTreeNode(n);
  }
  if (auto n = node->GetElement()) {
    str += EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitBindingPatternNode(BindingPatternNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "["s;

  for (unsigned i = 0; i < node->GetElementsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetElement(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += "]"s;

  if (auto n = node->GetType()) {
    str += ": "s + EmitTreeNode(n);
  }
  if (auto n = node->GetInit()) {
    str += "= "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitNumIndexSigNode(NumIndexSigNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetDataType()) {
    str += " "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitStrIndexSigNode(StrIndexSigNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetDataType()) {
    str += " "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitStructNode(StructNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  const char *suffix = ";\n";
  switch(node->GetProp()) {
    case SProp_CStruct:
      str = "struct "s;
      break;
    case SProp_TSInterface:
      str = "interface "s;
      break;
    case SProp_TSEnum:
      str = "enum "s;
      suffix = ",\n";
      break;
    case SProp_NA:
      str = "SProp_NA "s;
      break;
    default:
      MASSERT(0 && "Unexpected enumerator");
  }

  if (auto n = node->GetStructId()) {
    str += EmitIdentifierNode(n);
  }
  for (unsigned i = 0; i < node->GetSupersNum(); ++i) {
    str += i ? ", "s : " extends "s;
    if (auto n = node->GetSuper(i))
      str += EmitTreeNode(n);
  }
  str += " {\n"s;
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (auto n = node->GetField(i)) {
      str += EmitIdentifierNode(n) + suffix;
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
      str += EmitFunctionNode(n) + "\n"s;
    }
  }

  str += "}\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitFieldLiteralNode(FieldLiteralNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  auto lit = node->GetLiteral();
  if (auto n = node->GetFieldName()) {
    str = EmitTreeNode(n);
    if(lit)
      str += ": "s;
  }
  if(lit) {
    auto s = EmitTreeNode(lit);
    if(s.size() > 4 && (s[0] == 's' || s[0] == 'g') && !s.compare(1, 3, "et "))
      str = s;
    else
      str += s;
  }
  mPrecedence = '\030';
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitStructLiteralNode(StructLiteralNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "{"s;
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetField(i)) {
      str += EmitFieldLiteralNode(n);
    }
  }
  str += "}"s;
  mPrecedence = '\030';
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitNamespaceNode(NamespaceNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "namespace "s + node->GetName() + " {"s;
  for (unsigned i = 0; i < node->GetElementsNum(); ++i) {
    if (auto n = node->GetElementAtIndex(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += "}\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitVarListNode(VarListNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;

  for (unsigned i = 0; i < node->GetVarsNum(); ++i) {
    if (auto n = node->GetVarAtIndex(i)) {
      str += " "s + EmitIdentifierNode(n);
    }
  }

  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitExprListNode(ExprListNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;

  for (unsigned i = 0; i < node->GetExprsNum(); ++i) {
    if (auto n = node->GetExprAtIndex(i)) {
      str += " "s + EmitTreeNode(n);
    }
  }

  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitTemplateLiteralNode(TemplateLiteralNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "`"s;
  auto num = node->GetTreesNum();
  for (unsigned i = 0; i < num; ++i) {
    if (auto n = node->GetTreeAtIndex(i)) {
      std::string s(EmitTreeNode(n));
      str += i & 0x1 ? "${"s + s+ "}"s : s;
    }
  }
  str += "`"s;
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitLiteralNode(LiteralNode *node) {
  if (node == nullptr)
    return std::string();
  LitData lit = node->GetData();
  std::string str(AstDump::GetEnumLitData(lit));
  if(lit.mType == LT_StringLiteral || lit.mType == LT_CharacterLiteral) {
    str = "\"" + EncodeLiteral(str) + "\"";
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitRegExprNode(RegExprNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (const char* e = node->GetData().mExpr)
    str = "/"s + e + "/"s;
  if (const char* f = node->GetData().mFlags)
    str += f;
  return str;
}

std::string Emitter::EmitThrowNode(ThrowNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "throw "s;
  for (unsigned i = 0; i < node->GetExceptionsNum(); ++i) {
    if (auto n = node->GetExceptionAtIndex(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitCatchNode(CatchNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "catch("s;
  for (unsigned i = 0; i < node->GetParamsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetParamAtIndex(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += ")"s;
  if (auto n = node->GetBlock()) {
    str += EmitBlockNode(n);
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitFinallyNode(FinallyNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "finally "s;
  if (auto n = node->GetBlock()) {
    str += EmitBlockNode(n);
  }
  else
    str += "{}\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitTryNode(TryNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "try "s;
  if (auto n = node->GetBlock()) {
    str += EmitBlockNode(n);
  }
  for (unsigned i = 0; i < node->GetCatchesNum(); ++i) {
    if (auto n = node->GetCatchAtIndex(i)) {
      str += EmitCatchNode(n);
    }
  }
  if (auto n = node->GetFinally()) {
    str += EmitFinallyNode(n);
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitExceptionNode(ExceptionNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetException()) {
    str += " "s + EmitIdentifierNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitReturnNode(ReturnNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "return"s;
  if (auto n = node->GetResult()) {
    str += " "s + EmitTreeNode(n);
  }
  str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitCondBranchNode(CondBranchNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(auto n = node->GetLabel()) {
    str = EmitTreeNode(n) + ":\n"s;
  }
  str += "if("s;
  if (auto n = node->GetCond()) {
    auto cond = EmitTreeNode(n);
    str += Clean(cond);
  }
  str += ")"s;
  if (auto n = node->GetTrueBranch()) {
    str += EmitTreeNode(n);
  }
  if (auto n = node->GetFalseBranch()) {
    str += "else"s + EmitTreeNode(n);
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitBreakNode(BreakNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "break"s;
  if (auto n = node->GetTarget()) {
    str += " "s + EmitTreeNode(n);
  }
  str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitContinueNode(ContinueNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "continue"s;
  if (auto n = node->GetTarget()) {
    str += " "s + EmitTreeNode(n);
  }
  str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitForLoopNode(ForLoopNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(auto n = node->GetLabel()) {
    str = EmitTreeNode(n) + ":\n"s;
  }
  str += "for("s;
  switch(node->GetProp()) {
    case FLP_Regular:
      {
        for (unsigned i = 0; i < node->GetInitsNum(); ++i)
          if (auto n = node->GetInitAtIndex(i)) {
            auto init = EmitTreeNode(n);
            if (i)
              str += ", "s;
            str += Clean(init);
          }
        str += "; "s;
        if (auto n = node->GetCond()) {
          auto cond = EmitTreeNode(n);
          str += Clean(cond);
        }
        str += "; "s;
        for (unsigned i = 0; i < node->GetUpdatesNum(); ++i)
          if (auto n = node->GetUpdateAtIndex(i)) {
            auto update = EmitTreeNode(n);
            if (i)
              str += ", "s;
            str += Clean(update);
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
          str += EmitTreeNode(n);
        }
        str += " of "s;
        if (auto n = node->GetSet()) {
          str += EmitTreeNode(n);
        }
        break;
      }
    case FLP_NA:
      return "FLP_NA"s;
    default:
      MASSERT(0 && "Unexpected enumerator");
  }
  str += ")"s;

  if (auto n = node->GetBody()) {
    str += EmitTreeNode(n);
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitWhileLoopNode(WhileLoopNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(auto n = node->GetLabel()) {
    str = EmitTreeNode(n) + ":\n"s;
  }
  str += "while("s;
  if (auto n = node->GetCond()) {
    auto s = EmitTreeNode(n);
    str += Clean(s);
  }
  str += ")"s;
  if (auto n = node->GetBody()) {
    str += EmitTreeNode(n);
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitDoLoopNode(DoLoopNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(auto n = node->GetLabel()) {
    str = EmitTreeNode(n) + ":\n"s;
  }
  str += "do "s;
  if (auto n = node->GetBody()) {
    str += EmitTreeNode(n);
  }
  str += "while("s;
  if (auto n = node->GetCond()) {
    auto s = EmitTreeNode(n);
    str += Clean(s);
  }
  str += ");\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitSwitchLabelNode(SwitchLabelNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(node->IsDefault())
    str += "default:\n"s;
  if(auto n = node->GetValue()) {
    auto ce = EmitTreeNode(n);
    str += "case "s + Clean(ce) + ":\n";
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitSwitchCaseNode(SwitchCaseNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  for (unsigned i = 0; i < node->GetLabelsNum(); ++i) {
    if (auto n = node->GetLabelAtIndex(i)) {
      str += EmitTreeNode(n);
    }
  }
  for (unsigned i = 0; i < node->GetStmtsNum(); ++i) {
    if (auto t = node->GetStmtAtIndex(i))
      str += EmitTreeNode(t);
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitSwitchNode(SwitchNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if(auto n = node->GetLabel()) {
    str = EmitTreeNode(n) + ":\n"s;
  }
  str += "switch("s;
  if (auto n = node->GetExpr()) {
    auto expr = EmitTreeNode(n);
    str += Clean(expr);
  }
  str += "){\n"s;
  for (unsigned i = 0; i < node->GetCasesNum(); ++i) {
    if(auto n = node->GetCaseAtIndex(i))
      str += EmitTreeNode(n);
  }
  str += "}\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitAssertNode(AssertNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetExpr()) {
    str += " "s + EmitTreeNode(n);
  }
  if (auto n = node->GetMsg()) {
    str += " "s + EmitTreeNode(n);
  }
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitCallNode(CallNode *node) {
  if (node == nullptr)
    return std::string();
  // Function call: left-to-right, precedence = 20
  std::string str;
  if (auto n = node->GetMethod()) {
    auto s = EmitTreeNode(n);
    auto k = n->GetKind();
    if(k == NK_Function || k == NK_Lambda)
      str += "("s + s + ")"s;
    else
      str += s;
  }
  str += "("s;
  for (unsigned i = 0; i < node->GetArgsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetArg(i))
      str += EmitTreeNode(n);
  }
  str += ")"s;
  mPrecedence = '\024';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitInterfaceNode(InterfaceNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "interface "s + node->GetName();

  for (unsigned i = 0; i < node->GetSuperInterfacesNum(); ++i) {
    str += i ? ", "s : " extends "s;
    if (auto n = node->GetSuperInterfaceAtIndex(i))
      str += EmitTreeNode(n);
  }
  str += " {\n"s;

  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (auto n = node->GetFieldAtIndex(i)) {
      str += EmitIdentifierNode(n) + ";\n"s;
    }
  }

  for (unsigned i = 0; i < node->GetMethodsNum(); ++i) {
    if (auto n = node->GetMethodAtIndex(i)) {
      std::string func = EmitFunctionNode(n);
      str += func.length() > 2 && func.substr(func.length() - 2) == ";\n" ? func : func + ";\n"s;
    }
  }

  str += "}\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitClassNode(ClassNode *node) {
  if (node == nullptr)
    return std::string();

  std::string str;
  for (unsigned i = 0; i < node->GetAttributesNum(); ++i)
    str += AstDump::GetEnumAttrId(node->GetAttribute(i)) + " "s;

  str += "class "s + node->GetName();
  auto classNum = node->GetSuperClassesNum();
  for (unsigned i = 0; i < classNum; ++i) {
    str += i ? ", "s : " extends "s;
    if (auto n = node->GetSuperClass(i))
      str += EmitTreeNode(n);
  }
  for (unsigned i = 0; i < node->GetSuperInterfacesNum(); ++i) {
    str += i || classNum ? ", "s : " extends "s;
    if (auto n = node->GetSuperInterface(i))
      str += EmitTreeNode(n);
  }
  str += " {\n"s;

  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    if (auto n = node->GetField(i)) {
      str += EmitIdentifierNode(n) + ";\n"s;
    }
  }

  for (unsigned i = 0; i < node->GetConstructorsNum(); ++i) {
    if (auto n = node->GetConstructor(i)) {
      std::string func = EmitFunctionNode(n);
      Replace(func, "function", "constructor", 1);
      str += func;
    }
  }

  for (unsigned i = 0; i < node->GetMethodsNum(); ++i) {
    if (auto n = node->GetMethod(i)) {
      std::string func = EmitFunctionNode(n);
      str += func.length() > 2 && func.substr(func.length() - 2) == ";\n" ? func : func + ";\n"s;
    }
  }

  for (unsigned i = 0; i < node->GetInstInitsNum(); ++i) {
    if (auto n = node->GetInstInit(i)) {
      str += EmitBlockNode(n) + "\n"s;
    }
  }

  for (unsigned i = 0; i < node->GetLocalClassesNum(); ++i) {
    if (auto n = node->GetLocalClass(i)) {
      str += EmitClassNode(n) + "\n"s;
    }
  }

  for (unsigned i = 0; i < node->GetLocalInterfacesNum(); ++i) {
    if (auto n = node->GetLocalInterface(i)) {
      str += EmitInterfaceNode(n) + "\n"s;
    }
  }

  str += "}\n";
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitPassNode(PassNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str = "PassNode {"s;

  for (unsigned i = 0; i < node->GetChildrenNum(); ++i) {
    if (auto n = node->GetChild(i)) {
      str += " "s + EmitTreeNode(n);
    }
  }

  str += "}"s;
  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitLambdaNode(LambdaNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  switch (node->GetProperty()) {
    case LP_JSArrowFunction:
    case LP_TSFunctionType:
    case LP_TSConstructorType:
      break;
    case LP_JavaLambda:
    case LP_NA:
    default:
      MASSERT(0 && "Unexpected enumerator");
  }

  str += "("s;
  for (unsigned i = 0; i < node->GetParamsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetParam(i)) {
      str += EmitTreeNode(n);
    }
  }
  str += ")"s;

  if (auto n = node->GetBody()) {
    if (auto t = node->GetType()) {
      str += ": "s + EmitTreeNode(t);
    }
    str += " => "s + EmitTreeNode(n);
  }
  else {
    if (auto t = node->GetType()) {
      str += " => "s + EmitTreeNode(t);
    }
  }

  mPrecedence = '\030';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitInstanceOfNode(InstanceOfNode *node) {
  if (node == nullptr)
    return std::string();
  const Precedence precd = '\014';
  const bool rl_assoc = false;         // false: left-to-right
  std::string lhs, rhs;
  if (auto n = node->GetLeft()) {
    lhs = EmitTreeNode(n);
    if(precd > mPrecedence)
      lhs = "("s + lhs + ")"s;
  }
  else
    lhs = "(NIL) "s;
  if (auto n = node->GetRight()) {
    rhs = EmitTreeNode(n);
    if(precd > mPrecedence || (precd == mPrecedence && !rl_assoc))
      rhs = "("s + rhs + ")"s;
  }
  else
    rhs = " (NIL)"s;
  std::string str(lhs + " instanceof "s + rhs);
  mPrecedence = precd;
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitTypeOfNode(TypeOfNode *node) {
  if (node == nullptr)
    return std::string();
  const Precedence precd = '\121' & 0x3f;
  std::string str("typeof "s), rhs;
  if (auto n = node->GetExpr()) {
    rhs = EmitTreeNode(n);
    if(precd > mPrecedence) // right-to-left
      rhs = "("s + rhs + ")"s;
  }
  else
    rhs = " (NIL)"s;
  str += rhs;
  mPrecedence = precd;
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitKeyOfNode(KeyOfNode *node) {
  if (node == nullptr)
    return std::string();
  const Precedence precd = '\121' & 0x3f;
  std::string str("keyof "s), rhs;
  if (auto n = node->GetExpr()) {
    rhs = EmitTreeNode(n);
    if(precd > mPrecedence)
      rhs = "("s + rhs + ")"s;
  }
  else
    rhs = " (NIL)"s;
  str += rhs;
  mPrecedence = precd;
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}


std::string Emitter::EmitInNode(InNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetLeft()) {
    str += EmitTreeNode(n);
  }
  str += " in "s;
  if (auto n = node->GetRight()) {
    str += EmitTreeNode(n);
  }
  mPrecedence = '\014';
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitIsNode(IsNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetLeft()) {
    str = EmitTreeNode(n);
  }
  str += " is "s;
  if (auto n = node->GetRight()) {
    str += EmitTreeNode(n);
  }
  return str;
}

std::string Emitter::EmitModuleNode(ModuleNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str("// Filename: "s);
  str += node->GetFileName() + "\n"s;
  //str += AstDump::GetEnumSrcLang(node->GetSrcLang());
  /*
  if (auto n = node->GetPackage()) {
    str += " "s + EmitPackageNode(n);
  }

  for (unsigned i = 0; i < node->GetImportsNum(); ++i) {
    if (i)
      str += ", "s;
    if (auto n = node->GetImport(i)) {
      str += " "s + EmitImportNode(n);
    }
  }
  */
  for (unsigned i = 0; i < node->GetTreesNum(); ++i) {
    if (auto n = node->GetTree(i)) {
      str += EmitTreeNode(n);
    }
  }
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitAttrNode(AttrNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  str += " "s + AstDump::GetEnumAttrId(node->GetId());
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitPrimTypeNode(PrimTypeNode *node) {
  if (node == nullptr)
    return std::string();
  auto k = node->GetPrimType();
  return k == TY_None ? std::string() : Emitter::GetEnumTypeId(k);
}

std::string Emitter::EmitPrimArrayTypeNode(PrimArrayTypeNode *node) {
  if (node == nullptr)
    return std::string();
  std::string str;
  if (auto n = node->GetPrim()) {
    str += EmitPrimTypeNode(n);
  }
  if (auto n = node->GetDims()) {
    str += EmitDimensionNode(n);
    Replace(str, "never[", "[");
  }
  if (node->IsStmt())
    str += ";\n"s;
  return HandleTreeNode(str, node);
}

std::string Emitter::EmitTreeNode(TreeNode *node) {
  if (node == nullptr)
    return std::string();
  switch (node->GetKind()) {
  case NK_Module:
    return EmitModuleNode(static_cast<ModuleNode *>(node));
    break;
  case NK_Package:
    return EmitPackageNode(static_cast<PackageNode *>(node));
    break;
  case NK_XXportAsPair:
    return EmitXXportAsPairNode(static_cast<XXportAsPairNode *>(node));
    break;
  case NK_Import:
    return EmitImportNode(static_cast<ImportNode *>(node));
    break;
  case NK_Export:
    return EmitExportNode(static_cast<ExportNode *>(node));
    break;
  case NK_Declare:
    return EmitDeclareNode(static_cast<DeclareNode *>(node));
    break;
  case NK_Decl:
    return EmitDeclNode(static_cast<DeclNode *>(node));
    break;
  case NK_Identifier:
    return EmitIdentifierNode(static_cast<IdentifierNode *>(node));
    break;
  case NK_Field:
    return EmitFieldNode(static_cast<FieldNode *>(node));
    break;
  case NK_Dimension:
    return EmitDimensionNode(static_cast<DimensionNode *>(node));
    break;
  case NK_Attr:
    return EmitAttrNode(static_cast<AttrNode *>(node));
    break;
  case NK_PrimType:
    return EmitPrimTypeNode(static_cast<PrimTypeNode *>(node));
    break;
  case NK_PrimArrayType:
    return EmitPrimArrayTypeNode(static_cast<PrimArrayTypeNode *>(node));
    break;
  case NK_UserType:
    return EmitUserTypeNode(static_cast<UserTypeNode *>(node));
    break;
  case NK_TypeParameter:
    return EmitTypeParameterNode(static_cast<TypeParameterNode *>(node));
    break;
  case NK_AsType:
    return EmitAsTypeNode(static_cast<AsTypeNode *>(node));
    break;
  case NK_TypeAlias:
    return EmitTypeAliasNode(static_cast<TypeAliasNode *>(node));
    break;
  case NK_ConditionalType:
    return EmitConditionalTypeNode(static_cast<ConditionalTypeNode *>(node));
    break;
  case NK_Cast:
    return EmitCastNode(static_cast<CastNode *>(node));
    break;
  case NK_Parenthesis:
    return EmitParenthesisNode(static_cast<ParenthesisNode *>(node));
    break;
  case NK_BindingElement:
    return EmitBindingElementNode(static_cast<BindingElementNode *>(node));
    break;
  case NK_BindingPattern:
    return EmitBindingPatternNode(static_cast<BindingPatternNode *>(node));
    break;
  case NK_Struct:
    return EmitStructNode(static_cast<StructNode *>(node));
    break;
  case NK_StructLiteral:
    return EmitStructLiteralNode(static_cast<StructLiteralNode *>(node));
    break;
  case NK_FieldLiteral:
    return EmitFieldLiteralNode(static_cast<FieldLiteralNode *>(node));
    break;
  case NK_NumIndexSig:
    return EmitNumIndexSigNode(static_cast<NumIndexSigNode *>(node));
    break;
  case NK_StrIndexSig:
    return EmitStrIndexSigNode(static_cast<StrIndexSigNode *>(node));
    break;
  case NK_ArrayElement:
    return EmitArrayElementNode(static_cast<ArrayElementNode *>(node));
    break;
  case NK_ArrayLiteral:
    return EmitArrayLiteralNode(static_cast<ArrayLiteralNode *>(node));
    break;
  case NK_VarList:
    return EmitVarListNode(static_cast<VarListNode *>(node));
    break;
  case NK_ExprList:
    return EmitExprListNode(static_cast<ExprListNode *>(node));
    break;
  case NK_TemplateLiteral:
    return EmitTemplateLiteralNode(static_cast<TemplateLiteralNode *>(node));
    break;
  case NK_RegExpr:
    return EmitRegExprNode(static_cast<RegExprNode *>(node));
    break;
  case NK_Literal:
    return EmitLiteralNode(static_cast<LiteralNode *>(node));
    break;
  case NK_UnaOperator:
    return EmitUnaOperatorNode(static_cast<UnaOperatorNode *>(node));
    break;
  case NK_BinOperator:
    return EmitBinOperatorNode(static_cast<BinOperatorNode *>(node));
    break;
  case NK_TerOperator:
    return EmitTerOperatorNode(static_cast<TerOperatorNode *>(node));
    break;
  case NK_Lambda:
    return EmitLambdaNode(static_cast<LambdaNode *>(node));
    break;
  case NK_InstanceOf:
    return EmitInstanceOfNode(static_cast<InstanceOfNode *>(node));
    break;
  case NK_TypeOf:
    return EmitTypeOfNode(static_cast<TypeOfNode *>(node));
    break;
  case NK_KeyOf:
    return EmitKeyOfNode(static_cast<KeyOfNode *>(node));
    break;
  case NK_In:
    return EmitInNode(static_cast<InNode *>(node));
    break;
  case NK_Is:
    return EmitIsNode(static_cast<IsNode *>(node));
    break;
  case NK_Block:
    return EmitBlockNode(static_cast<BlockNode *>(node));
    break;
  case NK_Function:
    return EmitFunctionNode(static_cast<FunctionNode *>(node));
    break;
  case NK_Class:
    return EmitClassNode(static_cast<ClassNode *>(node));
    break;
  case NK_Interface:
    return EmitInterfaceNode(static_cast<InterfaceNode *>(node));
    break;
  case NK_Namespace:
    return EmitNamespaceNode(static_cast<NamespaceNode *>(node));
    break;
  case NK_AnnotationType:
    return EmitAnnotationTypeNode(static_cast<AnnotationTypeNode *>(node));
    break;
  case NK_Annotation:
    return EmitAnnotationNode(static_cast<AnnotationNode *>(node));
    break;
  case NK_Try:
    return EmitTryNode(static_cast<TryNode *>(node));
    break;
  case NK_Catch:
    return EmitCatchNode(static_cast<CatchNode *>(node));
    break;
  case NK_Finally:
    return EmitFinallyNode(static_cast<FinallyNode *>(node));
    break;
  case NK_Exception:
    return EmitExceptionNode(static_cast<ExceptionNode *>(node));
    break;
  case NK_Throw:
    return EmitThrowNode(static_cast<ThrowNode *>(node));
    break;
  case NK_Return:
    return EmitReturnNode(static_cast<ReturnNode *>(node));
    break;
  case NK_CondBranch:
    return EmitCondBranchNode(static_cast<CondBranchNode *>(node));
    break;
  case NK_Break:
    return EmitBreakNode(static_cast<BreakNode *>(node));
    break;
  case NK_Continue:
    return EmitContinueNode(static_cast<ContinueNode *>(node));
    break;
  case NK_ForLoop:
    return EmitForLoopNode(static_cast<ForLoopNode *>(node));
    break;
  case NK_WhileLoop:
    return EmitWhileLoopNode(static_cast<WhileLoopNode *>(node));
    break;
  case NK_DoLoop:
    return EmitDoLoopNode(static_cast<DoLoopNode *>(node));
    break;
  case NK_New:
    return EmitNewNode(static_cast<NewNode *>(node));
    break;
  case NK_Delete:
    return EmitDeleteNode(static_cast<DeleteNode *>(node));
    break;
  case NK_Call:
    return EmitCallNode(static_cast<CallNode *>(node));
    break;
  case NK_Assert:
    return EmitAssertNode(static_cast<AssertNode *>(node));
    break;
  case NK_SwitchLabel:
    return EmitSwitchLabelNode(static_cast<SwitchLabelNode *>(node));
    break;
  case NK_SwitchCase:
    return EmitSwitchCaseNode(static_cast<SwitchCaseNode *>(node));
    break;
  case NK_Switch:
    return EmitSwitchNode(static_cast<SwitchNode *>(node));
    break;
  case NK_Pass:
    return EmitPassNode(static_cast<PassNode *>(node));
    break;
  case NK_Null:
    // Ignore NullNode
    break;
  default:
    MASSERT(0 && "Unexpected node kind");
  }
  return std::string();
}

std::string &Emitter::HandleTreeNode(std::string &str, TreeNode *node) {
  auto num = node->GetAsTypesNum();
  if(num == 0)
    return str;
  str = "("s + str;
  for (unsigned i = 0; i < num; ++i)
    if (auto t = node->GetAsTypeAtIndex(i))
      str += EmitAsTypeNode(t);
  str += ")"s;
  return str;
}

std::string Emitter::GetEnumTypeId(TypeId k) {
  std::string str(AstDump::GetEnumTypeId(k) + 3);
  if (k != TY_Function && k != TY_Object)
    str[0] = std::tolower(str[0]);
  return str;
}

std::string Emitter::GetEnumDeclProp(DeclProp k) {
  std::string str(AstDump::GetEnumDeclProp(k) + 3);
  if(str != "NA")
    str[0] = std::tolower(str[0]);
  return str;
}

const char *Emitter::GetEnumOprId(OprId k) {
  // The first char in the returned string includes operator precedence and associativity info
  //
  // bits   7   6   5   4   3   2   1   0
  //        0   ^   ^---^---^-+-^---^---^
  //            |             |__ operator precedence
  //            |
  //            |__ associativity, 0: left-to-right, 1: right-to-left
  //
  switch (k) {
  case OPR_Plus:
    return          "\121+";
  case OPR_Add:
    return          "\016+";
  case OPR_Minus:
    return          "\121-";
  case OPR_Sub:
    return          "\016-";
  case OPR_Mul:
    return          "\017*";
  case OPR_Div:
    return          "\017/";
  case OPR_Mod:
    return          "\017%";
  case OPR_PreInc:
    return          "\121++";
  case OPR_Inc:
    return          "\022++";
  case OPR_PreDec:
    return          "\121--";
  case OPR_Dec:
    return          "\022--";
  case OPR_Exp:
    return          "\120**";
  case OPR_EQ:
    return          "\013==";
  case OPR_NE:
    return          "\013!=";
  case OPR_GT:
    return          "\014>";
  case OPR_LT:
    return          "\014<";
  case OPR_GE:
    return          "\014>=";
  case OPR_LE:
    return          "\014<=";
  case OPR_Band:
    return          "\012&";
  case OPR_Bor:
    return          "\010|";
  case OPR_Bxor:
    return          "\011^";
  case OPR_Bcomp:
    return          "\121~";
  case OPR_Shl:
    return          "\015<<";
  case OPR_Shr:
    return          "\015>>";
  case OPR_Zext:
    return          "\015>>>";
  case OPR_Land:
    return          "\007&&";
  case OPR_Lor:
    return          "\006||";
  case OPR_Not:
    return          "\121!";
  case OPR_Assign:
    return          "\103=";
  case OPR_AddAssign:
    return          "\103+=";
  case OPR_SubAssign:
    return          "\103-=";
  case OPR_MulAssign:
    return          "\103*=";
  case OPR_DivAssign:
    return          "\103/=";
  case OPR_ModAssign:
    return          "\103%=";
  case OPR_ShlAssign:
    return          "\103<<=";
  case OPR_ShrAssign:
    return          "\103>>=";
  case OPR_BandAssign:
    return          "\103&=";
  case OPR_BorAssign:
    return          "\103|=";
  case OPR_BxorAssign:
    return          "\103^=";
  case OPR_ZextAssign:
    return          "\103>>>=";
  case OPR_Arrow:
    return          "\030 OPR_Arrow";
  case OPR_Diamond:
    return          "\030 OPR_Diamond";
  case OPR_StEq:
    return          "\013===";
  case OPR_StNe:
    return          "\013!==";
  case OPR_ArrowFunction:
    return          "\030 OPR_ArrowFunction";
  case OPR_NullCoalesce:
    return          "\005??";
  case OPR_NA:
    return          "\030 OPR_NA";
  default:
    MASSERT(0 && "Unexpected enumerator");
  }
  return "UNEXPECTED OprId";
}

} // namespace maplefe
