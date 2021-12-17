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

#include <stack>
#include <set>
#include <algorithm>
#include <filesystem>
#include "ast_handler.h"
#include "ast_util.h"
#include "astopt.h"
#include "ast_xxport.h"
#include "gen_astdump.h"

namespace maplefe {

AST_XXport::AST_XXport(AstOpt *o, unsigned f) {
  mAstOpt = o;
  mASTHandler = o->GetASTHandler();
  mFlags = f;
}

unsigned AST_XXport::GetModuleNum() {
  return mASTHandler->GetSize();
}

void AST_XXport::BuildModuleOrder() {
  // setup module stridx
  SetModuleStrIdx();

  // collect dependent info
  CollectXXportNodes();

  // collect dependent info
  AddHandler();

  // sort handlers with dependency
  SortHandler();
}

void AST_XXport::SetModuleStrIdx() {
  for (int i = 0; i < GetModuleNum(); i++) {
    Module_Handler *handler = mASTHandler->GetModuleHandler(i);
    ModuleNode *module = handler->GetASTModule();

    // setup module node stridx with module file name
    unsigned stridx = gStringPool.GetStrIdx(module->GetFilename());
    module->SetStrIdx(stridx);
    mStrIdx2HandlerIdxMap[stridx] = i;
  }
}

TreeNode *AST_XXport::FindExportedDecl(unsigned hidx, unsigned stridx) {
  for (auto nid : mExportedDeclIds[hidx]) {
    TreeNode *node = mAstOpt->GetNodeFromNodeId(nid);
    if (node->GetStrIdx() == stridx) {
      return node;
    }
  }
  return NULL;
}

void AST_XXport::CollectXXportNodes() {
  for (int i = 0; i < GetModuleNum(); i++) {
    Module_Handler *handler = mASTHandler->GetModuleHandler(i);
    ModuleNode *module = handler->GetASTModule();

    XXportBasicVisitor visitor(this, handler, i, mFlags);
    visitor.Visit(module);
  }
}

void AST_XXport::AddHandler() {
  for (unsigned hidx = 0; hidx < GetModuleNum(); hidx++) {
    Module_Handler *handler = mASTHandler->GetModuleHandler(hidx);
    for (auto it : mImportNodeSets[hidx]) {
      ImportNode *node = it;
      UpdateDependency(hidx, node);
    }
    for (auto it : mExportNodeSets[hidx]) {
      ExportNode *node = it;
      UpdateDependency(hidx, node);
    }
  }
}

void AST_XXport::SortHandler() {
  for (int i = 0; i < GetModuleNum(); i++) {
    if (mHandlersIdxInOrder.size() == 0) {
      mHandlersIdxInOrder.push_back(i);
      continue;
    }

    bool added = false;
    std::list<unsigned>::iterator it = mHandlersIdxInOrder.begin();
    for (; it != mHandlersIdxInOrder.end(); it++) {
      unsigned idx = *it;
      // check if handler idx has dependency on i
      if (mHandlerIdx2DependentHandlerIdxMap[idx].find(i) != mHandlerIdx2DependentHandlerIdxMap[i].end()) {
        mHandlersIdxInOrder.insert(it, i);
        added = true;
        break;
      }
    }

    if (!added) {
      mHandlersIdxInOrder.push_back(i);
    }
  }

  // copy result to AstOpt
  std::list<unsigned>::iterator it = mHandlersIdxInOrder.begin();
  for (; it != mHandlersIdxInOrder.end(); it++) {
    unsigned idx = *it;
    Module_Handler *h = mASTHandler->GetModuleHandler(idx);
    mAstOpt->AddModuleHandler(h);
  }

  if (mFlags & FLG_trace_2) {
    std::cout << "============== Module Order ==============" << std::endl;
    std::list<unsigned>::iterator it = mHandlersIdxInOrder.begin();
    for (auto hidx: mHandlersIdxInOrder) {
      Module_Handler *handler = mASTHandler->GetModuleHandler(hidx);
      ModuleNode *module = handler->GetASTModule();
      std::cout << "module : " << gStringPool.GetStringFromStrIdx(module->GetStrIdx()) << std::endl;
      for (auto nid: mExportedDeclIds[hidx]) {
        TreeNode * node = mAstOpt->GetNodeFromNodeId(nid);
        std::cout << " export : " << gStringPool.GetStringFromStrIdx(node->GetStrIdx()) << " " << nid << std::endl;
      }
    }
  }
}

unsigned AST_XXport::GetHandleIdxFromStrIdx(unsigned stridx) {
  if (mStrIdx2HandlerIdxMap.find(stridx) != mStrIdx2HandlerIdxMap.end()) {
    return mStrIdx2HandlerIdxMap[stridx];
  }
  return FLG_no_imported ? 0 : DEFAULTVALUE;
}

void AST_XXport::CollectXXportInfo(unsigned hidx) {
  CollectImportInfo(hidx);
  CollectExportInfo(hidx);
}

// check if node is identifier with name "default"
static bool IsDefault(TreeNode *node) {
  return node->GetStrIdx() == gStringPool.GetStrIdx("default");
}

void AST_XXport::CollectImportInfo(unsigned hidx) {
  Module_Handler *handler = mASTHandler->GetModuleHandler(hidx);
  ModuleNode *module = handler->GetASTModule();

  for (auto it : mImportNodeSets[hidx]) {
    ImportNode *node = it;
    TreeNode *target = GetTarget(node);
    unsigned stridx = (target && target->GetStrIdx()) ? target->GetStrIdx() : module->GetStrIdx();
    XXportInfo *info = new XXportInfo(stridx, node->GetNodeId());;

    unsigned targethidx = DEFAULTVALUE;;
    Module_Handler *targethandler = NULL;
    ModuleNode *targetmodule = NULL;

    if (target) {
      targethidx = GetHandleIdxFromStrIdx(target->GetStrIdx());
      targethandler = mASTHandler->GetModuleHandler(targethidx);
      targetmodule = targethandler->GetASTModule();
    }

    for (unsigned i = 0; i < node->GetPairsNum(); i++) {
      XXportAsPairNode *p = node->GetPair(i);
      TreeNode *bfnode = p->GetBefore();
      TreeNode *afnode = p->GetAfter();
      MASSERT(bfnode && "before node NULL for default");

      // import * as MM from "./M";
      // bfnode represents a module
      if (p->IsEverything()) {
        info->SetEverything();

        MASSERT(target && "everything export no target");
        SetIdStrIdx2ModuleStrIdx(bfnode->GetStrIdx(), target->GetStrIdx());

        bfnode->SetTypeId(TY_Module);
        bfnode->SetTypeIdx(targetmodule->GetTypeIdx());

        std::pair<unsigned, unsigned> pnid(bfnode->GetNodeId(), 0);
        info->mNodeIdPairs.insert(pnid);
      } else {
        // reformat default import
        if (!p->IsDefault()) {
          if (IsDefault(bfnode) ) {
            p->SetIsDefault(true);
            p->SetBefore(afnode);
            p->SetAfter(NULL);
          }
        }

        bfnode = p->GetBefore();
        afnode = p->GetAfter();
        if (p->IsDefault()) {
          TreeNode *exported = GetExportedDefault(targetmodule->GetStrIdx());
          if (exported) {
            std::pair<unsigned, unsigned> pnid(exported->GetNodeId(), bfnode->GetNodeId());
            info->mNodeIdPairs.insert(pnid);
            TypeId tid = exported->GetTypeId();
            bfnode->SetTypeId(tid);
            bfnode->SetTypeIdx(tid);
          } else {
            NOTYETIMPL("failed to find the exported - default");
          }
        } else if (afnode) {
          // import bfnode as afnode
          TreeNode *exported = FindExportedDecl(targethidx, bfnode->GetStrIdx());
          if (!exported) {
            NOTYETIMPL("need to extract exported - bfnode M.x");
            exported = bfnode;
          }
          std::pair<unsigned, unsigned> pnid(exported->GetNodeId(), afnode->GetNodeId());
          info->mNodeIdPairs.insert(pnid);
          TypeId tid = exported->GetTypeId();
          bfnode->SetTypeId(tid);
          bfnode->SetTypeIdx(tid);
          afnode->SetTypeId(tid);
          afnode->SetTypeIdx(tid);
        } else if (bfnode) {
          // import bfnode
          TreeNode *exported = FindExportedDecl(targethidx, bfnode->GetStrIdx());
          if (!exported) {
            NOTYETIMPL("need to extract exported - bfnode M.x");
            exported = bfnode;
          }
          std::pair<unsigned, unsigned> pnid(exported->GetNodeId(), bfnode->GetNodeId());
          info->mNodeIdPairs.insert(pnid);
          TypeId tid = exported->GetTypeId();
          bfnode->SetTypeId(tid);
          bfnode->SetTypeIdx(tid);
        } else {
          NOTYETIMPL("failed to find the exported");
        }
      }
      // add afnode, bfnode as a decl
      if (afnode) {
        AddImportedDeclIds(handler->GetHidx(), afnode->GetNodeId());
        handler->AddNodeId2DeclMap(afnode->GetNodeId(), afnode);
      }
      AddImportedDeclIds(handler->GetHidx(), bfnode->GetNodeId());
      handler->AddNodeId2DeclMap(bfnode->GetNodeId(), bfnode);
    }

    mImports[hidx].insert(info);
  }
}

void AST_XXport::CollectExportInfo(unsigned hidx) {
  Module_Handler *handler = mASTHandler->GetModuleHandler(hidx);
  ModuleNode *module = handler->GetASTModule();

  for (auto it : mExportNodeSets[hidx]) {
    ExportNode *node = it;
    TreeNode *target = GetTarget(node);
    unsigned stridx = (target && target->GetStrIdx()) ? target->GetStrIdx() : module->GetStrIdx();
    XXportInfo *info = new XXportInfo(stridx, node->GetNodeId());;

    unsigned targethidx = DEFAULTVALUE;
    Module_Handler *targethandler = NULL;
    ModuleNode *targetmodule = NULL;

    if (target) {
      targethidx = GetHandleIdxFromStrIdx(target->GetStrIdx());
      targethandler = mASTHandler->GetModuleHandler(targethidx);
      targetmodule = targethandler->GetASTModule();
    }

    for (unsigned i = 0; i < node->GetPairsNum(); i++) {
      XXportAsPairNode *p = node->GetPair(i);
      TreeNode *bfnode = p->GetBefore();
      TreeNode *afnode = p->GetAfter();

      // export import a = M.a
      if (bfnode && bfnode->IsImport()) {
        ImportNode *imp = static_cast<ImportNode *>(bfnode);
        for (unsigned j = 0; j < imp->GetPairsNum(); j++) {
          XXportAsPairNode *q = imp->GetPair(i);
          TreeNode *bf = q->GetBefore();
          TreeNode *af = q->GetAfter();
          std::pair<unsigned, unsigned> pnid(af->GetNodeId(), bf->GetNodeId());
          info->mNodeIdPairs.insert(pnid);
        }
        continue;
      }

      if (p->IsEverything()) {
        info->SetEverything();
        if (bfnode) {
          // export * as MM from "./M";
          // bfnode represents a module
          MASSERT(target && "everything export no target");
          SetIdStrIdx2ModuleStrIdx(bfnode->GetStrIdx(), target->GetStrIdx());

          bfnode->SetTypeId(TY_Module);
          bfnode->SetTypeIdx(targetmodule->GetTypeIdx());
          AddExportedDeclIds(hidx, bfnode->GetNodeId());
        } else {
          // export * from "./M"
          for (auto k : mExportedDeclIds[targethidx]) {
            AddExportedDeclIds(hidx, k);
          }
          continue;
        }
      }

      // reformat default export
      if (p->IsDefault()) {
        p->SetIsRef(false);
      } else if (afnode && IsDefault(afnode)) {
        p->SetIsDefault(true);
        p->SetBefore(bfnode);
        p->SetAfter(NULL);
      }

      bfnode = p->GetBefore();
      afnode = p->GetAfter();
      unsigned exportednid = (afnode ? afnode->GetNodeId(): bfnode->GetNodeId());
      if (p->IsDefault()) {
        info->mDefaultNodeId = exportednid;
        AddExportedDeclIds(hidx, exportednid);
      } else if (mExportNodeSets[hidx].size() == 1 && node->GetPairsNum() == 1) {
        info->mDefaultNodeId = bfnode->GetNodeId();
        std::pair<unsigned, unsigned> pnid(bfnode->GetNodeId(), afnode ? afnode->GetNodeId() : 0);
        info->mNodeIdPairs.insert(pnid);
        AddExportedDeclIds(hidx, exportednid);
      } else {
        std::pair<unsigned, unsigned> pnid(bfnode->GetNodeId(), afnode ? afnode->GetNodeId() : 0);
        info->mNodeIdPairs.insert(pnid);
        AddExportedDeclIds(hidx, exportednid);
      }
    }

    mExports[hidx].insert(info);
  }
}

TreeNode *AST_XXport::GetTarget(TreeNode *node) {
  TreeNode *tree = NULL;
  if (node->IsImport()) {
    tree = static_cast<ImportNode *>(node)->GetTarget();
  } else if (node->IsExport()) {
    tree = static_cast<ExportNode *>(node)->GetTarget();
  }
  return tree;
}

// borrowed from ast2cpp
std::string AST_XXport::GetTargetFilename(unsigned hidx, TreeNode *node) {
  std::string filename;
  if (node && node->IsLiteral()) {
    LiteralNode *lit = static_cast<LiteralNode *>(node);
    LitData data = lit->GetData();
    filename = AstDump::GetEnumLitData(data);
    filename += ".ts"s;
    if(filename.front() != '/') {
      Module_Handler *handler = mASTHandler->GetModuleHandler(hidx);
      ModuleNode *module = handler->GetASTModule();
      std::filesystem::path p = module->GetFilename();
      try {
        p = std::filesystem::canonical(p.parent_path() / filename);
        filename = p.string();
      }
      catch(std::filesystem::filesystem_error const& ex) {
        // Ignore std::filesystem::filesystem_error exception
        // keep filename without converting it to a cannonical path
      }
    }
  }
  return filename;
}

// set up import/export node stridx with target module file name stridx
void AST_XXport::UpdateDependency(unsigned hidx, TreeNode *node) {
  TreeNode *target = GetTarget(node);
  if (target) {
    std::string name = GetTargetFilename(hidx, target);

    // store name's string index in node
    unsigned stridx = gStringPool.GetStrIdx(name);
    node->SetStrIdx(stridx);
    target->SetStrIdx(stridx);

    // update handler dependency map
    unsigned dep = GetHandleIdxFromStrIdx(stridx);
    mHandlerIdx2DependentHandlerIdxMap[hidx].insert(dep);
  }
}

unsigned AST_XXport::ExtractTargetStrIdx(TreeNode *node) {
  unsigned stridx = 0;
  if (node->IsField()) {
    FieldNode *fld = static_cast<FieldNode *>(node);
    TreeNode *upper = fld->GetUpper();
    stridx = GetModuleStrIdxFromIdStrIdx(upper->GetStrIdx());
    if (stridx) {
      unsigned hidx = GetHandleIdxFromStrIdx(stridx);
      Module_Handler *handler = mASTHandler->GetModuleHandler(hidx);
      ModuleNode *module = handler->GetASTModule();
      upper->SetTypeId(TY_Module);
      upper->SetTypeIdx(module->GetTypeIdx());
    }
  }
  return stridx;
}

// hstridx is the string index of handler/module name with full path
TreeNode *AST_XXport::GetExportedDefault(unsigned hstridx) {
  if (mStrIdx2HandlerIdxMap.find(hstridx) != mStrIdx2HandlerIdxMap.end()) {
    unsigned hidx = GetHandleIdxFromStrIdx(hstridx);
    for (auto it : mExports[hidx]) {
      if (it->mDefaultNodeId) {
        return mAstOpt->GetNodeFromNodeId(it->mDefaultNodeId);
      }
    }
  }
  return NULL;
}

// hidx is the index of handler, string is the string index of identifier
TreeNode *AST_XXport::GetExportedNamedNode(unsigned hidx, unsigned stridx) {
  for (auto it : mExports[hidx]) {
    for (auto it1 : it->mNodeIdPairs) {
      unsigned nid = it1.first;
      TreeNode *node = mAstOpt->GetNodeFromNodeId(nid);
      if (node->GetStrIdx() == stridx ) {
        return node;
      }
    }
  }
  return NULL;
}

// hidx is the index of handler, string is the string index of identifier
TreeNode *AST_XXport::GetExportedNodeFromImportedNode(unsigned hidx, unsigned nid) {
  TreeNode *node = NULL;

  for (auto it : mImports[hidx]) {
    if (it->mDefaultNodeId == nid) {
      TreeNode *node = GetExportedDefault(it->mModuleStrIdx);
      return node;
    }
    for (auto it1 : it->mNodeIdPairs) {
      unsigned nid2 = it1.second;
      if (nid2 == nid) {
        unsigned nid1 = it1.first;
        TreeNode *node1 = mAstOpt->GetNodeFromNodeId(nid1);
        return node1;
      }
    }
  }
  return NULL;
}

ImportNode *XXportBasicVisitor::VisitImportNode(ImportNode *node) {
  (void) AstVisitor::VisitImportNode(node);
  mASTXXport->mImportNodeSets[mHandlerIdx].push_back(node);

  TreeNode *target = mASTXXport->GetTarget(node);
  if (!target) {
    // extract target info for
    // import Bar = require("./Foo");
    for (unsigned i = 0; i < node->GetPairsNum(); i++) {
      XXportAsPairNode *p = node->GetPair(i);
      TreeNode *bfnode = p->GetBefore();
      if (bfnode && bfnode->IsLiteral()) {
        LiteralNode *lit = static_cast<LiteralNode *>(bfnode);
        LitId id = lit->GetData().mType;
        if (id == LT_StringLiteral) {
          node->SetTarget(bfnode);
          p->SetBefore(p->GetAfter());
          p->SetAfter(NULL);
          p->SetIsDefault(true);
        }
      }
    }
  }
  return node;
}

ExportNode *XXportBasicVisitor::VisitExportNode(ExportNode *node) {
  (void) AstVisitor::VisitExportNode(node);
  mASTXXport->mExportNodeSets[mHandlerIdx].push_back(node);
  return node;
}

}
