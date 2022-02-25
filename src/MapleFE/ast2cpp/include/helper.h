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

#ifndef __HELPER_H__
#define __HELPER_H__

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include "massert.h"
#include "ast.h"
#include "typetable.h"
#include "emitter.h"

using namespace std::string_literals;

namespace maplefe {

extern std::unordered_map<TypeId, std::string>TypeIdToJSType;
extern std::unordered_map<TypeId, std::string>TypeIdToJSTypeCXX;
extern TypeId hlpGetTypeId(TreeNode* node);
extern std::string GenClassFldAddProp(std::string, std::string, std::string, std::string, std::string);
extern std::string FunctionClassDecl(std::string retType, std::string funcName, unsigned nodeId);
extern std::string GeneratorClassDecl(std::string funcName, unsigned nodeId);
extern std::string GeneratorClassDef(std::string ns, std::string funcName, unsigned nodeId);
extern std::string tab(int n);
extern bool IsClassMethod(TreeNode* node);
extern std::string GetClassOfAssignedFunc(TreeNode* node);
extern std::string GenAnonFuncName(TreeNode* node);
inline std::string ClsName(std::string func) { return "Cls_"s + func; }
inline std::string GeneratorName(std::string func) { return "Generator_"s + func; }
inline std::string GeneratorFuncName(std::string func) { return "GeneratorFunc_"s + func; }
extern void HandleThisParam(unsigned nParams, TreeNode* node, std::string& params, std::string&args);
extern std::string hlpGetJSValTypeStr(TypeId typeId);
extern std::string ArrayCtorName(int dim, std::string type);
extern bool IsBuiltinObj(std::string name);

class FuncTable {
private:
  std::unordered_map<unsigned, TreeNode*> TopLevelFunc; // map nodeId to TreeNode*
  std::set<std::string> TopLevelFuncNm;   // name of top level func or name of var assigned top level func
  std::set<std::string> ImportedFields;
  std::set<std::string> StaticMembers;

  // map of FunctionNode node id to vector of function arg info (pair of arg type and name)
  std::unordered_map<unsigned, std::vector<std::pair<std::string, std::string>>> args;

public:
  FuncTable() {}
  ~FuncTable() {}

  // Check if node is top level function
  void AddTopLevelFunc(TreeNode* node) {
    assert(node->IsFunction());
    if (!static_cast<FunctionNode*>(node)->IsConstructor())
      TopLevelFunc[node->GetNodeId()] = node;
  }
  bool IsTopLevelFunc(TreeNode* node) {
    assert(node->IsFunction());
    std::unordered_map<unsigned, TreeNode*>::iterator it;
    it = TopLevelFunc.find(node->GetNodeId());
    return(it != TopLevelFunc.end());
  }

  // Check if name is top level func
  void AddNameIsTopLevelFunc(std::string name) {
    TopLevelFuncNm.insert(name); // name can be 1st level func, or func typed var
  }
  bool IsTopLevelFuncName(std::string& name) {
    return(TopLevelFuncNm.find(name) != TopLevelFuncNm.end());
  }

  // Check if string (xxx::yyy) is an Imported field
  std::string& AddFieldIsImported(std::string field) {
    ImportedFields.insert(field);
    return(field);
  }
  bool IsImportedField(std::string& field) {
    return(ImportedFields.find(field) != ImportedFields.end());
  }

  // Check if a class member (field or method) is static
  std::string& AddMemberIsStatic(std::string field) {
    StaticMembers.insert(field);
    return(field);
  }
  bool IsStaticMember(std::string& field) {
    return(StaticMembers.find(field) != StaticMembers.end());
  }

  // Function arg info
  void AddArgInfo(unsigned nodeId, std::string type, std::string name) {
    args[nodeId].push_back(std::pair(type, name));
  }
  std::vector<std::pair<std::string, std::string>> GetArgInfo(unsigned nodeId) {
    return args[nodeId];
  }

};

extern FuncTable hFuncTable;

}
#endif  // __HELPER_H__

