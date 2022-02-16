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
#include "ast_handler.h"
#include "ast_info.h"
#include "ast_util.h"
#include "ast_xxport.h"
#include "ast_ti.h"
#include "typetable.h"
#include "gen_astdump.h"

#define ITERATEMAX 10

namespace maplefe {

void TypeInfer::TypeInference() {
  ModuleNode *module = mHandler->GetASTModule();

  if (mFlags & FLG_trace_3) {
    gTypeTable.Dump();
  }

  // build mNodeId2Decl
  MSGNOLOC0("============== Build NodeId2Decl ==============");
  BuildIdNodeToDeclVisitor visitor_build(mHandler, mFlags, true);
  visitor_build.Visit(module);

  // type inference
  MSGNOLOC0("============== TypeInfer ==============");
  TypeInferVisitor visitor_ti(mHandler, mFlags, true);
  visitor_ti.SetUpdated(true);
  int count = 0;
  while (visitor_ti.GetUpdated() && count++ <= ITERATEMAX) {
    MSGNOLOC("\n TypeInference iterate ", count);
    visitor_ti.SetUpdated(false);
    visitor_ti.Visit(module);
  }

  // build mDirectFieldSet
  MSGNOLOC0("============== Build DirectFieldSet ==============");
  BuildIdDirectFieldVisitor visitor_field(mHandler, mFlags, true);
  visitor_field.Visit(module);
  if (mFlags & FLG_trace_3) {
    visitor_field.Dump();
  }

  if (mFlags & FLG_trace_3) std::cout << "\n>>>>>> TypeInference() iterated " << count << " times\n" << std::endl;

  // share UserType
  MSGNOLOC0("============== Share UserType ==============");
  ShareUTVisitor visitor_ut(mHandler, mFlags, true);
  visitor_ut.Push(module->GetRootScope());
  visitor_ut.Visit(module);

  // Check Type
  MSGNOLOC0("============== Check Type ==============");
  CheckTypeVisitor visitor_check(mHandler, mFlags, true);
  visitor_check.Visit(module);

  if (mFlags & FLG_trace_3) {
    mHandler->DumpArrayElemTypeIdMap();
  }
}

// build up mNodeId2Decl by visiting each Identifier
IdentifierNode *BuildIdNodeToDeclVisitor::VisitIdentifierNode(IdentifierNode *node) {
  if (mHandler->GetAstOpt()->IsLangKeyword(node)) {
    return node;
  }
  (void) AstVisitor::VisitIdentifierNode(node);
  // mHandler->FindDecl() will use/add entries to mNodeId2Decl
  TreeNode *decl = mHandler->FindDecl(node);
  if (decl) {
    mHandler->GetUtil()->SetTypeId(node, decl->GetTypeId());
    mHandler->GetUtil()->SetTypeIdx(node, decl->GetTypeIdx());
  }
  TreeNode *type = node->GetType();
  if (type && type->IsPrimType()) {
    PrimTypeNode *ptn = static_cast<PrimTypeNode *>(type);
    TypeId tid = ptn->GetPrimType();
    // mHandler->GetUtil()->SetTypeId(node, tid);
    mHandler->GetUtil()->SetTypeIdx(node, tid);
  }
  return node;
}

FieldNode *BuildIdDirectFieldVisitor::VisitFieldNode(FieldNode *node) {
  (void) AstVisitor::VisitFieldNode(node);
  IdentifierNode *field = static_cast<IdentifierNode *>(node->GetField());
  TreeNode *decl = NULL;
  decl = mHandler->FindDecl(field);
  if (decl) {
    mHandler->AddDirectField(field);
    mHandler->AddDirectField(node);
  }
  return node;
}

TreeNode *BuildIdDirectFieldVisitor::GetParentVarClass(TreeNode *node) {
  TreeNode *n = node;
  while (n && !n->IsModule()) {
    unsigned tyidx = 0;
    if (n->IsDecl()) {
      tyidx = n->GetTypeIdx();
    } else if (n->IsBinOperator()) {
      tyidx = n->GetTypeIdx();
    }
    if (tyidx) {
      return gTypeTable.GetTypeFromTypeIdx(tyidx);
    }
    n = n->GetParent();
  }
  return NULL;
}

FieldLiteralNode *BuildIdDirectFieldVisitor::VisitFieldLiteralNode(FieldLiteralNode *node) {
  (void) AstVisitor::VisitFieldLiteralNode(node);
  TreeNode *name = node->GetFieldName();
  IdentifierNode *field = static_cast<IdentifierNode *>(name);
  TreeNode *decl = mHandler->FindDecl(field);
  TreeNode *vtype = GetParentVarClass(decl);
  if (vtype) {
    // check if decl is a field of vtype
    // note: vtype could be in different module
    Module_Handler *h = mHandler->GetModuleHandler(vtype);
    TreeNode *fld = h->GetINFO()->GetField(vtype->GetNodeId(), decl->GetStrIdx());
    if (fld) {
      mHandler->AddDirectField(field);
      mHandler->AddDirectField(node);
    }
  }
  return node;
}

ArrayElementNode *BuildIdDirectFieldVisitor::VisitArrayElementNode(ArrayElementNode *node) {
  (void) AstVisitor::VisitArrayElementNode(node);
  TreeNode *array = node->GetArray();
  if (!array || !array->IsIdentifier()) {
    return node;
  }

  TreeNode *decl = mHandler->FindDecl(static_cast<IdentifierNode *>(array));
  if (decl && decl->IsTypeIdClass()) {
    // indexed access of types
    TreeNode *exp = node->GetExprAtIndex(0);
    unsigned stridx = exp->GetStrIdx();
    if (exp->IsLiteral() && exp->IsTypeIdString()) {
      stridx = (static_cast<LiteralNode *>(exp))->GetData().mData.mStrIdx;
    }
    if (decl->IsDecl()) {
      TreeNode *var = static_cast<DeclNode *>(decl)->GetVar();
      TreeNode * type = static_cast<IdentifierNode *>(var)->GetType();
      if (type && type->IsUserType()) {
        UserTypeNode *ut = static_cast<UserTypeNode *>(type);
        decl = mHandler->FindDecl(static_cast<IdentifierNode *>(ut->GetId()));
      }
    }
    if (decl->IsStruct() || decl->IsClass()) {
      for (int i = 0; i < mHandler->GetINFO()->GetFieldsSize(decl); i++) {
        TreeNode *f = mHandler->GetINFO()->GetField(decl, i);
        if (f->GetStrIdx() == stridx) {
          mHandler->AddDirectField(exp);
          mHandler->AddDirectField(node);
          return node;
        }
      }
    }
  }
  return node;
}

void BuildIdDirectFieldVisitor::Dump() {
  MSGNOLOC0("============== Direct Field NodeIds ==============");
  for (auto i: mHandler->mDirectFieldSet) {
    std::cout << " " << i;
  }
  std::cout << std::endl;
}

#undef  TYPE
#undef  PRIMTYPE
#define TYPE(T)
#define PRIMTYPE(T) case TY_##T:
bool TypeInferVisitor::IsPrimTypeId(TypeId tid) {
  bool result = false;
  switch (tid) {
#include "supported_types.def"
      result = true;
      break;
    default:
      break;
  }
  return result;
}

TypeId TypeInferVisitor::MergeTypeId(TypeId tia, TypeId tib) {
  if (tia == tib || tib == TY_None) {
    return tia;
  }

  if (tib == TY_Object || tib == TY_User) {
    return tib;
  }

  if ((tia == TY_Function && tib == TY_Class) || (tib == TY_Function && tia == TY_Class)) {
    return TY_None;
  }

  // tia != tib && tib != TY_None
  TypeId result = TY_None;
  switch (tia) {
    case TY_None:        result = tib;       break;

    case TY_Object:
    case TY_User:        result = tia;       break;

    case TY_Merge:
    case TY_Undefined:
    case TY_String:
    case TY_Function:
    case TY_Class:
    case TY_Array:       result = TY_Merge;  break;

    case TY_Boolean: {
      switch (tib) {
        case TY_Int:
        case TY_Long:
        case TY_Float:
        case TY_Double:  result = tib;       break;
        default:         result = TY_Merge;  break;
      }
      break;
    }
    case TY_Int: {
      switch (tib) {
        case TY_Boolean: result = TY_Int;    break;
        case TY_Long:
        case TY_Float:
        case TY_Double:  result = tib;       break;
        default:         result = TY_Merge;  break;
      }
      break;
    }
    case TY_Long: {
      switch (tib) {
        case TY_Boolean:
        case TY_Int:     result = TY_Long;   break;
        case TY_Float:
        case TY_Double:  result = TY_Double; break;
        default:         result = TY_Merge;  break;
      }
      break;
    }
    case TY_Float: {
      switch (tib) {
        case TY_Boolean:
        case TY_Int:     result = TY_Float;  break;
        case TY_Long:
        case TY_Double:  result = TY_Double; break;
        default:         result = TY_Merge;  break;
      }
      break;
    }
    case TY_Double: {
      switch (tib) {
        case TY_Boolean:
        case TY_Int:
        case TY_Long:
        case TY_Double:  result = TY_Double; break;
        default:         result = TY_Merge;  break;
      }
      break;
    }
    default:
      break;
  }
  if (result == TY_None) {
    NOTYETIMPL("MergeTypeId()");
  }
  if (mFlags & FLG_trace_3) {
    std::cout << " Type Merge: "
              << AstDump::GetEnumTypeId(tia) << " "
              << AstDump::GetEnumTypeId(tib) << " --> "
              << AstDump::GetEnumTypeId(result) << std::endl;
  }
  return result;
}

unsigned TypeInferVisitor::MergeTypeIdx(unsigned tia, unsigned tib) {
  if (tia == tib || tib == 0) {
    return tia;
  }

  if (tia == 0) {
    return tib;
  }

  unsigned result = 0;

  TreeNode *ta = gTypeTable.GetTypeFromTypeIdx(tia);
  TreeNode *tb = gTypeTable.GetTypeFromTypeIdx(tib);
  if (ta->IsPrimType() && tb->IsPrimType()) {
    TypeId tid = MergeTypeId(ta->GetTypeId(), tb->GetTypeId());
    result = (unsigned)tid;
  } else {
    TreeNode *type = gTypeTable.GetTypeFromTypeId(TY_Merge);
    result = type->GetTypeIdx();
  }

  if (mFlags & FLG_trace_3) {
    std::cout << " Type idx Merge: "
              << tia << " "
              << tib << " --> "
              << result << std::endl;
  }
  return result;
}

void TypeInferVisitor::SetTypeId(TreeNode *node, TypeId tid) {
  TypeId id = node->GetTypeId();
  mHandler->GetUtil()->SetTypeId(node, tid);
  if (tid != id) {
    id = node->GetTypeId();
    if (IsPrimTypeIdx(id) && node->GetTypeIdx() == 0) {
      SetTypeIdx(node, id);
    }
    SetUpdated();
  }
}

void TypeInferVisitor::SetTypeIdx(TreeNode *node, unsigned tidx) {
  mHandler->GetUtil()->SetTypeIdx(node, tidx);
  if (node && node->GetTypeIdx() != tidx) {
    SetUpdated();
  }
}

void TypeInferVisitor::SetTypeId(TreeNode *node1, TreeNode *node2) {
  SetTypeId(node1, node2->GetTypeId());
  SetTypeIdx(node1, node2->GetTypeIdx());
}

void TypeInferVisitor::SetTypeIdx(TreeNode *node1, TreeNode *node2) {
  SetTypeIdx(node1, node2->GetTypeIdx());
}

void TypeInferVisitor::UpdateTypeId(TreeNode *node, TypeId tid) {
  if (tid == TY_None || !node || node->IsLiteral()) {
    return;
  }
  tid = MergeTypeId(node->GetTypeId(), tid);
  SetTypeId(node, tid);
}

void TypeInferVisitor::UpdateTypeId(TreeNode *node1, TreeNode *node2) {
  if (!node1 || !node2 || node1 == node2) {
    return;
  }
  TypeId tid = MergeTypeId(node1->GetTypeId(), node2->GetTypeId());
  if (!node1->IsLiteral()) {
    SetTypeId(node1, tid);
  }
  if (!node2->IsLiteral()) {
    SetTypeId(node2, tid);
  }

  // update type idx as well
  UpdateTypeIdx(node1, node2);
}

void TypeInferVisitor::UpdateTypeIdx(TreeNode *node, unsigned tidx) {
  if (tidx == 0 || !node || node->IsLiteral()) {
    return;
  }
  tidx = MergeTypeIdx(node->GetTypeIdx(), tidx);
  SetTypeIdx(node, tidx);
}

void TypeInferVisitor::UpdateTypeIdx(TreeNode *node1, TreeNode *node2) {
  if (!node1 || !node2 || node1 == node2) {
    return;
  }
  unsigned tidx = MergeTypeIdx(node1->GetTypeIdx(), node2->GetTypeIdx());
  if (tidx != 0) {
    SetTypeIdx(node1, tidx);
    SetTypeIdx(node2, tidx);
  }
}

PrimTypeNode *TypeInferVisitor::GetOrClonePrimTypeNode(PrimTypeNode *pt, TypeId tid) {
  PrimTypeNode *new_pt = pt;
  TypeId oldtid = pt->GetTypeId();
  // merge tids
  tid = MergeTypeId(oldtid, tid);
  // check if we need update
  if (tid != oldtid) {
    // check if we need clone PrimTypeNode to avoid using the shared one
    if (oldtid == TY_None) {
      new_pt = mHandler->NewTreeNode<PrimTypeNode>();
      new_pt->SetPrimType(pt->GetPrimType());
    }
    SetTypeId(new_pt, tid);
    SetTypeIdx(new_pt, tid);
    SetUpdated();
  }
  return new_pt;
}

bool static IsScalar(TypeId tid) {
  switch (tid) {
    case TY_None:
    case TY_Int:
    case TY_Long:
    case TY_Float:
    case TY_Double:
      return true;
    default:
      return false;
  }
  return false;
}

// when function arry type parameter is updated, need update all
// caller arguments to be consistent with the array type parameter
void TypeInferVisitor::UpdateArgArrayDecls(unsigned nid, TypeId tid) {
  for (auto id: mParam2ArgArrayDeclMap[nid]) {
    mHandler->SetArrayElemTypeId(nid, tid);
    if (id && id->IsDecl()) {
      id = static_cast<DeclNode *>(id)->GetVar();
    }
    if (id && id->IsIdentifier()) {
      IdentifierNode *inode = static_cast<IdentifierNode *>(id);
      TreeNode *type = inode->GetType();
      if (type && type->IsPrimArrayType()) {
        PrimArrayTypeNode *pat = static_cast<PrimArrayTypeNode *>(type);
        SetTypeId(pat->GetPrim(), tid);
        SetUpdated();
      }
    }
  }
}

// use input node's type info to update target node's type info
void TypeInferVisitor::UpdateTypeUseNode(TreeNode *target, TreeNode *input) {
  // this functionality is reserved for typescript
  if (!mHandler->IsTS()) {
    return;
  }
  TypeId tid = target->GetTypeId();
  TypeId iid = input->GetTypeId();
  if ((tid == iid && IsScalar(tid)) || (iid == TY_Array && tid != iid)) {
    return;
  }
  switch (iid) {
    case TY_None:
      break;
    case TY_Array: {
      // function's formals with corresponding calls' parameters passed in
      if (input->IsIdentifier()) {
        TreeNode *decl = mHandler->FindDecl(static_cast<IdentifierNode *>(input));
        TypeId old_elemTypeId = GetArrayElemTypeId(target);
        unsigned old_elemTypeIdx = GetArrayElemTypeIdx(target);
        TypeId inid = GetArrayElemTypeId(decl);
        unsigned inidx = GetArrayElemTypeIdx(decl);
        if (old_elemTypeId != inid || old_elemTypeIdx != inidx) {
          UpdateArrayElemTypeMap(target, inid, inidx);
        }
        TypeId new_elemTypeId = GetArrayElemTypeId(target);
        TreeNode *type = static_cast<IdentifierNode *>(target)->GetType();
        MASSERT(target->IsIdentifier() && "target node not identifier");
        if (type->IsPrimArrayType()) {
          unsigned nid = target->GetNodeId();
          mParam2ArgArrayDeclMap[nid].insert(decl);
          if (old_elemTypeId != new_elemTypeId) {
            PrimArrayTypeNode *pat = static_cast<PrimArrayTypeNode *>(type);
            PrimTypeNode *pt = pat->GetPrim();
            PrimTypeNode *new_pt = GetOrClonePrimTypeNode(pt, new_elemTypeId);
            pat->SetPrim(new_pt);
            SetUpdated();

            UpdateArgArrayDecls(nid, new_elemTypeId);
          }
        }
      }
      // function's return type with return statement
      else if (input->IsArrayLiteral()) {
        TypeId old_elemTypeId = GetArrayElemTypeId(target);
        unsigned old_elemTypeIdx = GetArrayElemTypeIdx(target);
        TypeId inid = GetArrayElemTypeId(input);
        unsigned inidx = GetArrayElemTypeIdx(input);
        if (old_elemTypeId != inid || old_elemTypeIdx != inidx) {
          UpdateArrayElemTypeMap(target, inid, inidx);
        }
        TypeId new_elemTypeId = GetArrayElemTypeId(target);
        if (target->IsPrimArrayType()) {
          unsigned nid = target->GetNodeId();
          if (old_elemTypeId != new_elemTypeId) {
            PrimArrayTypeNode *pat = static_cast<PrimArrayTypeNode *>(target);
            PrimTypeNode *pt = pat->GetPrim();
            PrimTypeNode *new_pt = GetOrClonePrimTypeNode(pt, new_elemTypeId);
            pat->SetPrim(new_pt);
            SetUpdated();

            UpdateArgArrayDecls(nid, new_elemTypeId);
          }
        }
      } else {
        NOTYETIMPL("parameter not identifier");
      }
      break;
    }
    case TY_Int:
    case TY_Long:
    case TY_Float:
    case TY_Double:
    case TY_String:
    case TY_Class:
    case TY_Object:
    case TY_Function: {
      TypeId merged = MergeTypeId(tid, iid);
      if (merged != tid && merged != TY_None) {
        SetTypeId(target, merged);
        SetUpdated();
      }
      break;
    }
    case TY_User:
      break;
    default:
      NOTYETIMPL("TypeId not handled");
      break;
  }
  return;
}

void TypeInferVisitor::UpdateFuncRetTypeId(FunctionNode *node, TypeId tid, unsigned tidx) {
  if (!node || (node->GetTypeId() == tid && node->GetTypeIdx() == tidx)) {
    return;
  }
  TreeNode *type = node->GetType();
  // create new return type node if it was shared

  if (type) {
    if (type->IsPrimType() && type->IsTypeIdNone()) {
      type = GetOrClonePrimTypeNode((PrimTypeNode *)type, tid);
      node->SetType(type);
    }
    tid = MergeTypeId(type->GetTypeId(), tid);
    SetTypeId(type, tid);
    tidx = MergeTypeIdx(type->GetTypeIdx(), tidx);
    SetTypeIdx(type, tidx);
  }
}

TypeId TypeInferVisitor::GetArrayElemTypeId(TreeNode *node) {
  unsigned nid = node->GetNodeId();
  return mHandler->GetArrayElemTypeId(nid);
}

unsigned TypeInferVisitor::GetArrayElemTypeIdx(TreeNode *node) {
  unsigned nid = node->GetNodeId();
  return mHandler->GetArrayElemTypeIdx(nid);
}

void TypeInferVisitor::UpdateArrayElemTypeMap(TreeNode *node, TypeId tid, unsigned tidx) {
  if (!node || tid == TY_None || !IsArray(node)) {
    return;
  }
  unsigned nodeid = node->GetNodeId();
  TypeId currtid = mHandler->GetArrayElemTypeId(nodeid);
  tid = MergeTypeId(tid, currtid);

  unsigned currtidx = mHandler->GetArrayElemTypeIdx(nodeid);
  tidx = MergeTypeIdx(tidx, currtidx);

  if (currtid != tid || currtidx != tidx) {
    mHandler->SetArrayElemTypeId(node->GetNodeId(), tid);
    mHandler->SetArrayElemTypeIdx(node->GetNodeId(), tidx);
    SetUpdated();

    // update array's PrimType node with a new node
    if (node->IsDecl()) {
      DeclNode *decl = static_cast<DeclNode *>(node);
      node = decl->GetVar();
    }
    if (node->IsIdentifier()) {
      IdentifierNode *in = static_cast<IdentifierNode *>(node);
      node = in->GetType();
    }

    if (node->IsPrimArrayType()) {
      PrimArrayTypeNode *pat = static_cast<PrimArrayTypeNode *>(node);
      PrimTypeNode *pt = pat->GetPrim();
      PrimTypeNode *new_pt = GetOrClonePrimTypeNode(pt, tid);
      pat->SetPrim(new_pt);
    }
  }
}

void TypeInferVisitor::UpdateArrayDimMap(TreeNode *node, DimensionNode *dim) {
  mHandler->SetArrayDim(node->GetNodeId(), dim);
}

// return true if identifier is constructor
bool TypeInferVisitor::UpdateVarTypeWithInit(TreeNode *var, TreeNode *init) {
  bool result = var->IsFunction();
  if (!var->IsIdentifier()) {
    return result;
  }
  IdentifierNode *idnode = static_cast<IdentifierNode *>(var);
  TreeNode *type = idnode->GetType();
  // use init NewNode to set decl type
  if (!type && init) {
    if (init->IsNew()) {
      NewNode *n = static_cast<NewNode *>(init);
      if (n->GetId()) {
        TreeNode *id = n->GetId();
        if (id->IsIdentifier() && id->IsTypeIdClass()) {
          UserTypeNode *utype = mInfo->CreateUserTypeNode(id->GetStrIdx(), var->GetScope());
          utype->SetParent(idnode);
          idnode->SetType(utype);
          SetUpdated();
        }
      }
    } else if (init->IsIdentifier()) {
      TreeNode *decl = mHandler->FindDecl(static_cast<IdentifierNode *>(init));
      if (decl) {
        unsigned tidx = decl->GetTypeIdx();
        if ((decl->IsClass() || (0 < tidx && tidx < (unsigned)TY_Max))) {
          SetTypeId(idnode, TY_Function);
          SetUpdated();
          result = true;
        }
      }
    } else if (init->IsArrayLiteral()) {
      TypeId tid = GetArrayElemTypeId(init);
      unsigned tidx = GetArrayElemTypeIdx(init);
      if (IsPrimTypeId(tid)) {
        PrimTypeNode *pt = mHandler->NewTreeNode<PrimTypeNode>();
        pt->SetPrimType(tid);

        PrimArrayTypeNode *pat = mHandler->NewTreeNode<PrimArrayTypeNode>();
        pat->SetPrim(pt);

        DimensionNode *dims = mHandler->GetArrayDim(init->GetNodeId());
        pat->SetDims(dims);

        pat->SetParent(idnode);
        idnode->SetType(pat);
        SetUpdated();
      } else if (tidx != 0) {
        TreeNode *t = gTypeTable.GetTypeFromTypeIdx(tidx);
        UserTypeNode *utype = mInfo->CreateUserTypeNode(t->GetStrIdx(), var->GetScope());

        ArrayTypeNode *pat = mHandler->NewTreeNode<ArrayTypeNode>();
        pat->SetElemType(utype);

        DimensionNode *dims = mHandler->GetArrayDim(init->GetNodeId());
        pat->SetDims(dims);

        pat->SetParent(idnode);
        idnode->SetType(pat);
        SetUpdated();
      }
    }
  }
  return result;
}

bool TypeInferVisitor::IsArray(TreeNode *node) {
  if (!node) {
    return false;
  }
  if (node->IsArrayLiteral() || node->IsPrimArrayType()) {
    return true;
  }
  TreeNode *tn = node;
  if (node->IsDecl()) {
    DeclNode *decl = static_cast<DeclNode *>(node);
    tn = decl->GetVar();
  }
  if (tn->IsIdentifier()) {
    IdentifierNode *idnode = static_cast<IdentifierNode *>(tn);
    if (idnode && idnode->GetType() && idnode->GetType()->IsPrimArrayType()) {
      return true;
    }
  } else if (tn->IsBindingPattern()) {
    // could be either object or array destructuring
    return false;
  } else {
    NOTYETIMPL("array not identifier or bind pattern");
  }
  return false;
}

TreeNode *TypeInferVisitor::VisitClassField(TreeNode *node) {
  (void) AstVisitor::VisitTreeNode(node);
  if (node->IsIdentifier()) {
    IdentifierNode *idnode = static_cast<IdentifierNode *>(node);
    TreeNode *type = idnode->GetType();
    if (type) {
      TypeId tid = type->GetTypeId();
      if (type->IsPrimType()) {
        PrimTypeNode *ptn = static_cast<PrimTypeNode *>(type);
        tid = ptn->GetPrimType();
      }
      // use non TY_Number
      if (tid != TY_Number) {
        SetTypeId(node, tid);
      }
    }
    TreeNode *init = idnode->GetInit();
    if (init) {
      VisitTreeNode(init);
      UpdateTypeId(node, init->GetTypeId());
    }
  } else if (node->IsLiteral()) {
    MSGNOLOC0("field is Literal");
  } else if (node->IsComputedName()) {
    MSGNOLOC0("field is ComputedName");
  } else if (node->IsStrIndexSig()) {
    MSGNOLOC0("field is StrIndexSig");
  } else {
    NOTYETIMPL("field new kind");
  }
  return node;
}

// ArrayElementNode are for
// 1. array access
// 2. indexed access of class/structure for fields, field types
ArrayElementNode *TypeInferVisitor::VisitArrayElementNode(ArrayElementNode *node) {
  (void) AstVisitor::VisitArrayElementNode(node);
  TreeNode *array = node->GetArray();
  if (array) {
    if (array->IsIdentifier()) {
      TreeNode *decl = mHandler->FindDecl(static_cast<IdentifierNode *>(array));
      if (decl) {
        // indexed access of class fields or types
        if (decl->IsTypeIdClass()) {
          SetTypeId(array, TY_Class);
          TreeNode *exp = node->GetExprAtIndex(0);
          if (exp->IsLiteral()) {
            if (exp->IsTypeIdString()) {
              // indexed access of types
              unsigned stridx = (static_cast<LiteralNode *>(exp))->GetData().mData.mStrIdx;
              if (decl->IsDecl()) {
                TreeNode *var = static_cast<DeclNode *>(decl)->GetVar();
                TreeNode * type = static_cast<IdentifierNode *>(var)->GetType();
                if (type && type->IsUserType()) {
                  UserTypeNode *ut = static_cast<UserTypeNode *>(type);
                  decl = mHandler->FindDecl(static_cast<IdentifierNode *>(ut->GetId()));
                }
              }
              if (decl->IsStruct() || decl->IsClass()) {
                bool found = false;
                for (int i = 0; i < mInfo->GetFieldsSize(decl); i++) {
                  TreeNode *f = mInfo->GetField(decl, i);
                  if (f->GetStrIdx() == stridx) {
                    UpdateTypeId(node, f);
                    found = true;
                    break;
                  }
                }
                // new field
                if (!found) {
                  IdentifierNode *id = mInfo->CreateIdentifierNode(stridx);
                  mInfo->AddField(decl, id);
                }
              }
            } else if (exp->IsTypeIdInt()) {
              // indexed access of fields
              // unsigned i = (static_cast<LiteralNode *>(exp))->GetData().mData.mInt64;
              NOTYETIMPL("indexed access with literal field id");
            } else {
              AstVisitor::VisitTreeNode(exp);
              NOTYETIMPL("indexed access not literal");
            }
          } else {
            AstVisitor::VisitTreeNode(exp);
          }
        } else {
          // default
          UpdateTypeId(array, TY_Array);
          UpdateTypeId(decl, array);
          UpdateArrayElemTypeMap(decl, node->GetTypeId(), node->GetTypeIdx());
          UpdateTypeId(node, mHandler->GetArrayElemTypeId(decl->GetNodeId()));
        }
      } else {
        NOTYETIMPL("array not declared");
      }
    } else if (array->IsArrayElement()) {
      NOTYETIMPL("array in ArrayElementNode IsArrayElement");
    } else if (array->IsField()) {
      NOTYETIMPL("array in ArrayElementNode IsField");
    } else if (array->IsUserType()) {
      NOTYETIMPL("array in ArrayElementNode IsUserType");
    } else if (array->IsBinOperator()) {
      NOTYETIMPL("array in ArrayElementNode IsBinOperator");
    } else if (array->IsLiteral() && ((LiteralNode*)array)->IsThis()) {
      NOTYETIMPL("array in ArrayElementNode IsLiteral");
    } else if (array->IsPrimType()) {
      NOTYETIMPL("array in ArrayElementNode IsPrimType");
    } else {
      NOTYETIMPL("array in ArrayElementNode unknown");
    }
  }
  return node;
}

FieldLiteralNode *TypeInferVisitor::VisitFieldLiteralNode(FieldLiteralNode *node) {
  (void) AstVisitor::VisitFieldLiteralNode(node);
  TreeNode *name = node->GetFieldName();
  TreeNode *lit = node->GetLiteral();
  UpdateTypeId(name, lit->GetTypeId());
  UpdateTypeId(node, name);
  return node;
}

ArrayLiteralNode *TypeInferVisitor::VisitArrayLiteralNode(ArrayLiteralNode *node) {
  UpdateTypeId(node, TY_Array);
  (void) AstVisitor::VisitArrayLiteralNode(node);
  ArrayLiteralNode *al = node;
  if (node->IsArrayLiteral()) {
    al = static_cast<ArrayLiteralNode *>(node);
    unsigned size = al->GetLiteralsNum();
    TypeId tid = TY_None;
    unsigned tidx = 0;
    bool allElemArray = true;
    for (unsigned i = 0; i < size; i++) {
      TreeNode *n = al->GetLiteral(i);
      TypeId id = n->GetTypeId();
      unsigned idx = n->GetTypeIdx();
      tid = MergeTypeId(tid, id);
      tidx = MergeTypeIdx(tidx, idx);
      if (tid != TY_Array) {
        allElemArray = false;
      }
    }

    DimensionNode *dim = mHandler->NewTreeNode<DimensionNode>();
    dim->AddDimension(size);

    // n-D array: elements are all arrays
    if (allElemArray) {
      unsigned elemdim = DEFAULTVALUE;
      // recalculate element typeid
      tid = TY_None;
      unsigned tidx = 0;
      for (unsigned i = 0; i < size; i++) {
        TreeNode *n = al->GetLiteral(i);
        if (n->IsArrayLiteral()) {
          DimensionNode * dn = mHandler->GetArrayDim(n->GetNodeId());
          unsigned currdim = dn ? dn->GetDimensionsNum() : 0;
          // find min dim of all elements
          if (elemdim == DEFAULTVALUE) {
            elemdim = currdim;
            tid = mHandler->GetArrayElemTypeId(n->GetNodeId());
            tidx = mHandler->GetArrayElemTypeIdx(n->GetNodeId());
          } else if (currdim < elemdim) {
            elemdim = currdim;
            tid = TY_Merge;
            tidx = 0;
          } else if (currdim > elemdim) {
            tid = TY_Merge;
            tidx = 0;
          } else {
            tid = MergeTypeId(tid, mHandler->GetArrayElemTypeId(n->GetNodeId()));
            tidx = MergeTypeIdx(tidx, mHandler->GetArrayElemTypeIdx(n->GetNodeId()));
          }
        }
      }
      if (elemdim != DEFAULTVALUE) {
        for (unsigned i = 0; i < elemdim; i++) {
          // with unspecified length, can add details later
          dim->AddDimension(0);
        }
      }
    }

    UpdateArrayElemTypeMap(node, tid, tidx);
    UpdateArrayDimMap(node, dim);
  }

  return node;
}

BinOperatorNode *TypeInferVisitor::VisitBinOperatorNode(BinOperatorNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting BinOperatorNode, id=" << node->GetNodeId() << "..." << std::endl;
  // (void) AstVisitor::VisitBinOperatorNode(node);
  OprId op = node->GetOprId();
  TreeNode *ta = node->GetOpndA();
  TreeNode *tb = node->GetOpndB();
  (void) VisitTreeNode(tb);
  (void) VisitTreeNode(ta);
  // modified operand
  TreeNode *mod = NULL;
  TypeId tia = ta->GetTypeId();
  TypeId tib = tb->GetTypeId();
  switch (op) {
    case OPR_StEq:
    case OPR_EQ:
    case OPR_NE:
    case OPR_GT:
    case OPR_LT:
    case OPR_GE:
    case OPR_LE: {
      if (tia != TY_None && tib == TY_None) {
        UpdateTypeId(tb, tia);
        mod = tb;
      } else if (tia == TY_None && tib != TY_None) {
        UpdateTypeId(ta, tib);
        mod = ta;
      }
      SetTypeId(node, TY_Boolean);
      SetTypeIdx(node, TY_Boolean);
      break;
    }
    case OPR_Assign:
    case OPR_AddAssign:
    case OPR_SubAssign:
    case OPR_MulAssign:
    case OPR_DivAssign:
    case OPR_ModAssign:
    case OPR_ShlAssign:
    case OPR_ShrAssign:
    case OPR_BandAssign:
    case OPR_BorAssign:
    case OPR_BxorAssign:
    case OPR_ZextAssign: {
      TypeId ti = MergeTypeId(tia, tib);
      if (tia == TY_None || (ta->IsIdentifier() && tia != ti)) {
        UpdateTypeId(ta, ti);
        mod = ta;
      } else if (tib == TY_None) {
        UpdateTypeId(tb, ti);
        mod = tb;
      }
      UpdateTypeId(node, ti);
      unsigned tix = MergeTypeIdx(ta->GetTypeIdx(), tb->GetTypeIdx());
      UpdateTypeIdx(ta, tix);
      UpdateTypeIdx(tb, tix);
      UpdateTypeIdx(node, tix);
      break;
    }
    case OPR_Add:
    case OPR_Sub:
    case OPR_Mul:
    case OPR_Div: {
      if (tia != TY_None && tib == TY_None) {
        UpdateTypeId(tb, tia);
        mod = tb;
      } else if (tia == TY_None && tib != TY_None) {
        UpdateTypeId(ta, tib);
        mod = ta;
      }
      TypeId ti = MergeTypeId(tia, tib);
      UpdateTypeId(node, ti);
      unsigned tix = MergeTypeIdx(ta->GetTypeIdx(), tb->GetTypeIdx());
      UpdateTypeIdx(ta, tix);
      UpdateTypeIdx(tb, tix);
      UpdateTypeIdx(node, tix);
      break;
    }
    case OPR_Mod:
    case OPR_Band:
    case OPR_Bor:
    case OPR_Bxor:
    case OPR_Shl:
    case OPR_Shr:
    case OPR_Zext: {
      SetTypeId(ta, TY_Int);
      SetTypeId(tb, TY_Int);
      SetTypeId(node, TY_Int);
      SetTypeIdx(ta, TY_Int);
      SetTypeIdx(tb, TY_Int);
      SetTypeIdx(node, TY_Int);
      break;
    }
    case OPR_Land:
    case OPR_Lor: {
      SetTypeId(ta, TY_Boolean);
      SetTypeId(tb, TY_Boolean);
      SetTypeId(node, TY_Boolean);
      SetTypeIdx(ta, TY_Boolean);
      SetTypeIdx(tb, TY_Boolean);
      SetTypeIdx(node, TY_Boolean);
      break;
    }
    case OPR_Exp: {
      if (tia == TY_Int && tib == TY_Int) {
        SetTypeId(node, TY_Int);
        SetTypeIdx(node, TY_Int);
      }
      break;
    }
    case OPR_NullCoalesce:
      break;
    default: {
      NOTYETIMPL("VisitBinOperatorNode()");
      break;
    }
  }
  // visit mod to update its content
  (void) VisitTreeNode(mod);
  return node;
}

CallNode *TypeInferVisitor::VisitCallNode(CallNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting CallNode, id=" << node->GetNodeId() << "..." << std::endl;
  TreeNode *method = node->GetMethod();
  Module_Handler *handler = mHandler;
  UpdateTypeId(method, TY_Function);
  if (method) {
    if (method->IsField()) {
      FieldNode *field = static_cast<FieldNode *>(method);
      method = field->GetField();
      TreeNode *upper = field->GetUpper();
      handler = mAstOpt->GetHandlerFromNodeId(upper->GetNodeId());
    }
    if (method->IsIdentifier()) {
      IdentifierNode *mid = static_cast<IdentifierNode *>(method);
      TreeNode *decl = mHandler->FindDecl(mid);
      if (decl) {
        SetTypeIdx(node, decl->GetTypeIdx());
        SetTypeIdx(mid, decl->GetTypeIdx());

        if (decl->IsDecl()) {
          DeclNode *d = static_cast<DeclNode *>(decl);
          if (d->GetInit()) {
            decl = d->GetInit();
          }
        }
        if (decl && decl->IsIdentifier()) {
          IdentifierNode *id = static_cast<IdentifierNode *>(decl);
          if (id->GetType()) {
            decl = id->GetType();
          } else if (id->IsTypeIdFunction()) {
            NOTYETIMPL("VisitCallNode TY_Function");
          }
        }
        if (decl) {
          if (decl->IsFunction()) {
            FunctionNode *func = static_cast<FunctionNode *>(decl);
            // check if called a generator
            if (func->IsGenerator()) {
              mHandler->AddGeneratorUse(node->GetNodeId(), func);
            }
            // update call's return type
            if (func->GetType()) {
              UpdateTypeId(node, func->GetType()->GetTypeId());
            }
            // skip imported and exported functions as they are generic
            // so should not restrict their types
            if (!mXXport->IsImportedExportedDeclId(mHandler->GetHidx(), decl->GetNodeId())) {
              unsigned min = func->GetParamsNum();
              if (func->GetParamsNum() != node->GetArgsNum()) {
                // count minimun number of args need to be passed
                min = 0;
                // check arg about whether it is optional or has default value
                for (unsigned i = 0; i < func->GetParamsNum(); i++) {
                  TreeNode *arg = func->GetParam(i);
                  if (arg->IsOptional()) {
                    continue;
                  } else if(arg->IsIdentifier()) {
                    IdentifierNode *id = static_cast<IdentifierNode *>(arg);
                    TreeNode *d = mHandler->FindDecl(id);
                    if (d) {
                      SetTypeId(id, d->GetTypeId());
                      SetTypeIdx(id, d->GetTypeIdx());
                    }
                    if (!id->GetInit()) {
                      min++;
                    }
                  } else {
                    min++;
                  }
                }
                if (min > node->GetArgsNum()) {
                  NOTYETIMPL("call and func number of arguments not compatible");
                  return node;
                }
              }
              // update function's argument types
              for (unsigned i = 0; i < min; i++) {
                UpdateTypeUseNode(func->GetParam(i), node->GetArg(i));
              }

              // dummy functions like console.log
              if (func->IsTypeIdNone()) {
                for (unsigned i = 0; i < node->GetArgsNum(); i++) {
                  TreeNode *arg = node->GetArg(i);
                  if(arg->IsIdentifier()) {
                    IdentifierNode *id = static_cast<IdentifierNode *>(arg);
                    TreeNode *d = mHandler->FindDecl(id);
                    if (d) {
                      SetTypeId(id, d->GetTypeId());
                      SetTypeIdx(id, d->GetTypeIdx());
                    }
                  }
                }
              }
            }
          } else if (decl->IsCall()) {
            (void) VisitCallNode(static_cast<CallNode *>(decl));
          } else if (decl->IsDecl()) {
            DeclNode *d = static_cast<DeclNode *>(decl);
            if (d->GetInit()) {
              NOTYETIMPL("VisitCallNode decl init");
            }
          } else if (decl->IsLiteral()) {
            NOTYETIMPL("VisitCallNode literal node");
          } else {
            NOTYETIMPL("VisitCallNode not function node");
          }
        } else {
          NOTYETIMPL("VisitCallNode null decl");
        }
      } else if (mAstOpt->IsLangKeyword(mid)) {
        // known calls
      } else {
        // calling constructor like Array(...) could also end up here
        TreeNode *type = mHandler->FindType(mid);
        if (type) {
          NOTYETIMPL("VisitCallNode type");
        } else {
          NOTYETIMPL("VisitCallNode null decl and null type");
        }
      }
    }
  }
  (void) AstVisitor::VisitCallNode(node);
  return node;
}

CastNode *TypeInferVisitor::VisitCastNode(CastNode *node) {
  (void) AstVisitor::VisitCastNode(node);
  TreeNode *dest = node->GetDestType();
  SetTypeId(node, dest);
  return node;
}

AsTypeNode *TypeInferVisitor::VisitAsTypeNode(AsTypeNode *node) {
  (void) AstVisitor::VisitAsTypeNode(node);
  TreeNode *dest = node->GetType();
  SetTypeId(node, dest);

  TreeNode *parent = node->GetParent();
  if (parent) {
    // pass to parent, need refine if multiple AsTypeNode
    if (parent->GetAsTypesNum() == 1 && parent->GetAsTypeAtIndex(0) == node) {
      SetTypeId(parent, dest);
    }
  }
  return node;
}

ClassNode *TypeInferVisitor::VisitClassNode(ClassNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting ClassNode, id=" << node->GetNodeId() << "..." << std::endl;
  UpdateTypeId(node, TY_Class);
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    TreeNode *t = node->GetField(i);
    (void) VisitClassField(t);
  }
  (void) AstVisitor::VisitClassNode(node);
  return node;
}

CondBranchNode *TypeInferVisitor::VisitCondBranchNode(CondBranchNode *node) {
  (void) AstVisitor::VisitCondBranchNode(node);
  TreeNode *cond = node->GetCond();
  TreeNode *blockT = NULL;
  TreeNode *blockF = NULL;
  if (cond->IsUnaOperator()) {
    cond = static_cast<UnaOperatorNode *>(cond)->GetOpnd();
    blockT = node->GetFalseBranch();
    blockF = node->GetTrueBranch();
  } else {
    blockT = node->GetTrueBranch();
    blockF = node->GetFalseBranch();
  }

  if (cond->IsCall()) {
    CallNode *call = static_cast<CallNode *>(cond);
    TreeNode *method = call->GetMethod();
    if (method && method->IsIdentifier()) {
      IdentifierNode *id = static_cast<IdentifierNode *>(method);
      unsigned mid = id->GetStrIdx();
      unsigned nid = node->GetNodeId();
      // check if already visited for mid and nodeid
      if (mCbFuncIsDone.find(mid) != mCbFuncIsDone.end() &&
          mCbFuncIsDone[mid].find(nid) != mCbFuncIsDone[mid].end()) {
        return node;
      }
      TreeNode *decl = mHandler->FindDecl(id);
      if (decl && decl->IsFunction()) {
        unsigned fid = decl->GetNodeId();
        if (mFuncIsNodeMap.find(fid) != mFuncIsNodeMap.end()) {
          unsigned tidx = mFuncIsNodeMap[fid];
          TreeNode *arg = call->GetArg(0);
          if (arg->IsIdentifier()) {
            unsigned stridx = arg->GetStrIdx();
            mChangeTypeIdxVisitor->Setup(stridx, tidx);
            mChangeTypeIdxVisitor->Visit(blockT);
            mCbFuncIsDone[mid].insert(nid);
            SetUpdated();

            // if union of 2 types, update other branch with other type
            TreeNode *argdecl = mHandler->FindDecl(static_cast<IdentifierNode *>(arg));
            if (argdecl && argdecl->IsIdentifier()) {
              TreeNode *type = static_cast<IdentifierNode *>(argdecl)->GetType();
              if (type->IsUserType()) {
                UserTypeNode *ut = static_cast<UserTypeNode *>(type);
                if (ut->GetType() == UT_Union && ut->GetUnionInterTypesNum() == 2) {
                  TreeNode *u0 = ut->GetUnionInterType(0);
                  TreeNode *u1 = ut->GetUnionInterType(1);
                  tidx = (u0->GetTypeIdx() == tidx) ? u1->GetTypeIdx() : u0->GetTypeIdx();
                  mChangeTypeIdxVisitor->Setup(stridx, tidx);
                  mChangeTypeIdxVisitor->Visit(blockF);
                }
              }
            }
          }
        }
      }

    } else {
      NOTYETIMPL("mentod null or not identifier");
    }
  }

  return node;
}

DeclNode *TypeInferVisitor::VisitDeclNode(DeclNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting DeclNode, id=" << node->GetNodeId() << "..." << std::endl;
  (void) AstVisitor::VisitDeclNode(node);
  TreeNode *init = node->GetInit();
  TreeNode *var = node->GetVar();
  TypeId merged = node->GetTypeId();
  unsigned mergedtidx = node->GetTypeIdx();
  TypeId elemTypeId = TY_None;
  unsigned elemTypeIdx = 0;
  bool isArray = false;
  if (init) {
    merged = MergeTypeId(merged, init->GetTypeId());
    mergedtidx = MergeTypeIdx(mergedtidx, init->GetTypeIdx());
    // collect array element typeid if any
    elemTypeId = GetArrayElemTypeId(init);
    elemTypeIdx = GetArrayElemTypeIdx(init);
    isArray = (elemTypeId != TY_None);
    // pass IsGeneratorUse
    mHandler->UpdateGeneratorUse(node->GetNodeId(), init->GetNodeId());
    if (var) {
      mHandler->UpdateGeneratorUse(var->GetNodeId(), init->GetNodeId());
    }
  }
  if (var) {
    // normal cases
    if(var->IsIdentifier()) {
      merged = MergeTypeId(merged, var->GetTypeId());
      mergedtidx = MergeTypeIdx(mergedtidx, var->GetTypeIdx());
      bool isFunc = UpdateVarTypeWithInit(var, init);
      if (isFunc) {
        UpdateTypeId(node, var->GetTypeId());
        UpdateTypeIdx(node, mergedtidx);
        UpdateTypeIdx(var, mergedtidx);
        return node;
      }
    } else {
      // BindingPatternNode
    }
  } else {
    MASSERT("var null");
  }
  // override TypeId for array
  if (isArray) {
    merged = TY_Array;
    SetTypeId(node, merged);
    SetTypeId(init, merged);
    SetTypeId(var, merged);
  } else {
    UpdateTypeId(node, merged);
    UpdateTypeId(init, merged);
    UpdateTypeId(var, merged);
    if (mergedtidx > 0) {
      UpdateTypeIdx(node, mergedtidx);
      UpdateTypeIdx(init, mergedtidx);
      UpdateTypeIdx(var, mergedtidx);
    }
  }
  SetTypeIdx(node, var->GetTypeIdx());
  if (isArray || IsArray(node)) {
    UpdateArrayElemTypeMap(node, elemTypeId, elemTypeIdx);
  }
  return node;
}

ImportNode *TypeInferVisitor::VisitImportNode(ImportNode *node) {
  //(void) AstVisitor::VisitImportNode(node);
  TreeNode *target = node->GetTarget();
  unsigned hidx = DEFAULTVALUE;
  unsigned hstridx = 0;
  if (target) {
    std::string name = mXXport->GetTargetFilename(mHandler->GetHidx(), target);
    // store name's string index in node
    hstridx = gStringPool.GetStrIdx(name);
    hidx = mXXport->GetHandleIdxFromStrIdx(hstridx);
  }
  for (unsigned i = 0; i < node->GetPairsNum(); i++) {
    XXportAsPairNode *p = node->GetPair(i);
    TreeNode *bfnode = p->GetBefore();
    TreeNode *afnode = p->GetAfter();
    if (bfnode) {
      if (hidx == DEFAULTVALUE) {
        hstridx = mXXport->ExtractTargetStrIdx(bfnode);
        if (hstridx) {
          hidx = mXXport->GetHandleIdxFromStrIdx(hstridx);
        } else {
          NOTYETIMPL("can not find import target");
          return node;
        }
      }
      if (p->IsDefault()) {
        TreeNode *dflt = mXXport->GetExportedDefault(hstridx);
        if (dflt) {
          UpdateTypeId(bfnode, dflt->GetTypeId());
          UpdateTypeIdx(bfnode, dflt->GetTypeIdx());
        } else {
          NOTYETIMPL("can not find exported default");
        }
      } else if (!bfnode->IsTypeIdModule()) {
        TreeNode *exported = NULL;
        if (bfnode->IsField()) {
          FieldNode *field = static_cast<FieldNode *>(bfnode);
          TreeNode *upper = field->GetUpper();
          TreeNode *fld = field->GetField();
          if (upper->IsTypeIdModule()) {
            TreeNode *type = gTypeTable.GetTypeFromTypeIdx(upper->GetTypeIdx());
            Module_Handler *h = mHandler->GetModuleHandler(type);
            exported = mXXport->GetExportedNamedNode(h->GetHidx(), fld->GetStrIdx());
            if (exported) {
              UpdateTypeId(bfnode, exported->GetTypeId());
              UpdateTypeIdx(bfnode, exported->GetTypeIdx());
            }
          }
        } else {
          exported = mXXport->GetExportedNamedNode(hidx, bfnode->GetStrIdx());
          if (exported) {
            SetTypeId(bfnode, exported);
          }
        }
        if (!exported) {
          NOTYETIMPL("can not find exported node");
        }
      }

      SetTypeId(p, bfnode);
      if (afnode) {
        SetTypeId(afnode, bfnode);
      }
    }
  }
  return node;
}

// check if node is identifier with name "default"+RENAMINGSUFFIX
static bool IsDefault(TreeNode *node) {
  return node->GetStrIdx() == gStringPool.GetStrIdx(std::string("default") + RENAMINGSUFFIX);
}

ExportNode *TypeInferVisitor::VisitExportNode(ExportNode *node) {
  (void) AstVisitor::VisitExportNode(node);
  unsigned hidx = mHandler->GetHidx();
  for (unsigned i = 0; i < node->GetPairsNum(); i++) {
    XXportAsPairNode *p = node->GetPair(i);
    TreeNode *bfnode = p->GetBefore();
    TreeNode *afnode = p->GetAfter();
    if (bfnode) {
      switch (bfnode->GetKind()) {
        case NK_Struct:
        case NK_Function:
        case NK_Decl: {
          mXXport->AddExportedDeclIds(hidx, bfnode->GetNodeId());
          break;
        }
        case NK_Declare: {
          DeclareNode *declare = static_cast<DeclareNode *>(bfnode);
          for (unsigned i = 0; i < declare-> GetDeclsNum(); i++) {
            TreeNode *decl = declare->GetDeclAtIndex(i);
            if (decl) {
              mXXport->AddExportedDeclIds(hidx, decl->GetNodeId());
            }
          }
          break;
        }
        case NK_Identifier: {
          IdentifierNode *idnode = static_cast<IdentifierNode *>(bfnode);
          TreeNode *decl = mHandler->FindDecl(idnode);
          if (decl) {
            mXXport->AddExportedDeclIds(hidx, decl->GetNodeId());
          }
          if (IsDefault(bfnode)) {
            unsigned stridx = node->GetStrIdx();
            TreeNode *dflt = mXXport->GetExportedDefault(stridx);
            if (dflt) {
              UpdateTypeId(bfnode, dflt->GetTypeId());
              UpdateTypeIdx(bfnode, dflt->GetTypeIdx());
            } else {
              NOTYETIMPL("can not find exported default");
            }
          }
          break;
        }
        case NK_TypeAlias:
        case NK_UserType:
        case NK_Import:
          break;
        default: {
          NOTYETIMPL("new export node kind");
          break;
        }
      }

      UpdateTypeId(p, bfnode->GetTypeId());
      UpdateTypeIdx(p, bfnode->GetTypeIdx());
      if (afnode) {
        UpdateTypeId(afnode, bfnode->GetTypeId());
        UpdateTypeIdx(afnode, bfnode->GetTypeIdx());
      }
    }
  }
  return node;
}

FieldNode *TypeInferVisitor::VisitFieldNode(FieldNode *node) {
  (void) AstVisitor::VisitFieldNode(node);
  TreeNode *upper = node->GetUpper();
  IdentifierNode *field = static_cast<IdentifierNode *>(node->GetField());
  TreeNode *decl = NULL;
  if (!upper) {
    decl = mHandler->FindDecl(field);
  } else {
    if (upper->IsLiteral()) {
      LiteralNode *ln = static_cast<LiteralNode *>(upper);
      // this.f
      if (ln->GetData().mType == LT_ThisLiteral) {
        decl = mHandler->FindDecl(field);
      }
    }
    if (!decl) {
      unsigned tidx = upper->GetTypeIdx();
      if (tidx) {
        TreeNode *n = gTypeTable.GetTypeFromTypeIdx(tidx);
        ASTScope *scope = n->GetScope();
        decl = scope->FindDeclOf(field->GetStrIdx());
      }
    }
  }
  if (decl) {
    UpdateTypeId(node, decl);
  }
  UpdateTypeId(field, node);
  return node;
}

ForLoopNode *TypeInferVisitor::VisitForLoopNode(ForLoopNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting ForLoopNode, id=" << node->GetNodeId() << "..." << std::endl;
  if (node->GetProp() == FLP_JSIn) {
    TreeNode *var = node->GetVariable();
    if (var) {
      SetTypeId(var, TY_Int);
    }
  }
  (void) AstVisitor::VisitForLoopNode(node);
  return node;
}

FunctionNode *TypeInferVisitor::VisitFunctionNode(FunctionNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting FunctionNode, id=" << node->GetNodeId() << "..." << std::endl;
  UpdateTypeId(node, node->IsArray() ? TY_Object : TY_Function);
  (void) AstVisitor::VisitFunctionNode(node);
  if (node->GetFuncName()) {
    SetTypeId(node->GetFuncName(), node->GetTypeId());
  }
  if (node->GetType()) {
    SetTypeIdx(node, node->GetType()->GetTypeIdx());
  }
  return node;
}

IdentifierNode *TypeInferVisitor::VisitIdentifierNode(IdentifierNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting IdentifierNode, id=" << node->GetNodeId() << "..." << std::endl;
  if (mAstOpt->IsLangKeyword(node)) {
    return node;
  }
  TreeNode *type = node->GetType();
  if (type) {
    if (type->IsPrimArrayType()) {
      SetTypeId(node, TY_Array);
      SetUpdated();
    }
  }
  (void) AstVisitor::VisitIdentifierNode(node);
  if (type) {
    unsigned tidx = type->GetTypeIdx();
    if (tidx && node->GetTypeIdx() != tidx) {
      UpdateTypeId(node, type->GetTypeId());
      UpdateTypeIdx(node, tidx);
      SetUpdated();
    }
  }
  if (node->GetInit()) {
    UpdateTypeId(node, node->GetInit()->GetTypeId());
    UpdateTypeIdx(node, node->GetInit()->GetTypeIdx());
    SetUpdated();
    return node;
  }
  TreeNode *parent = node->GetParent();
  TreeNode *decl = NULL;
  if (parent && parent->IsField()) {
    FieldNode *field = static_cast<FieldNode *>(parent);
    TreeNode *upper = field->GetUpper();
    TreeNode *fld = field->GetField();
    ASTScope *scope = NULL;
    if (node == upper) {
      if (upper->IsThis()) {
        // this.f
        scope = upper->GetScope();
        // this is the parent with typeid TY_Class
        while (scope && scope->GetTree()->GetTypeId() != TY_Class) {
          scope = scope->GetParent();
        }
        if (scope) {
          upper->SetTypeId(scope->GetTree()->GetTypeId());
          upper->SetTypeIdx(scope->GetTree()->GetTypeIdx());
        }
        decl = upper;
      } else {
        decl = mHandler->FindDecl(node, true);
      }
    } else if (node == fld) {
      TreeNode *uptype = gTypeTable.GetTypeFromTypeIdx(upper->GetTypeIdx());
      if (uptype) {
        scope = uptype->GetScope();
        node->SetScope(scope);
        decl = mHandler->FindDecl(node, true);
      }
    } else {
      NOTYETIMPL("node not in field");
    }
  } else {
    decl = mHandler->FindDecl(node);
  }

  if (decl) {
    // check node itself is part of decl
    if (decl != parent) {
      UpdateTypeId(node, decl);
      UpdateTypeIdx(node, decl);
    }
  } else {
    NOTYETIMPL("node not declared");
    MSGNOLOC0(node->GetName());
  }

  return node;
}

InterfaceNode *TypeInferVisitor::VisitInterfaceNode(InterfaceNode *node) {
  UpdateTypeId(node, TY_Class);
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    TreeNode *t = node->GetField(i);
    (void) VisitClassField(t);
  }
  (void) AstVisitor::VisitInterfaceNode(node);
  return node;
}

IsNode *TypeInferVisitor::VisitIsNode(IsNode *node) {
  (void) AstVisitor::VisitIsNode(node);
  SetTypeIdx(node, TY_Boolean);
  TreeNode *parent = node->GetParent();
  if (parent->IsFunction()) {
    FunctionNode *func = static_cast<FunctionNode *>(parent);
    if (func->GetType() == node) {
      TreeNode *right = node->GetRight();
      if (right->IsUserType()) {
        TreeNode *id = static_cast<UserTypeNode *>(right)->GetId();
        SetTypeIdx(right, id->GetTypeIdx());
        mFuncIsNodeMap[func->GetNodeId()] = id->GetTypeIdx();
      } else {
        NOTYETIMPL("isnode right not user type");
      }
    }
  }

  return node;
}

NewNode *TypeInferVisitor::VisitNewNode(NewNode *node) {
  (void) AstVisitor::VisitNewNode(node);
  TreeNode *id = node->GetId();
  if (id) {
    UpdateTypeId(node, TY_Class);
    if (id->GetTypeIdx() == 0) {
      if (id->IsIdentifier()) {
        IdentifierNode *idn = static_cast<IdentifierNode *>(id);
        TreeNode *decl = mHandler->FindDecl(idn);
        if (decl && decl->GetTypeIdx() != 0) {
          SetTypeIdx(node, decl->GetTypeIdx());
        }
      }
    } else {
      SetTypeIdx(node, id->GetTypeIdx());
    }
  }
  return node;
}

StructNode *TypeInferVisitor::VisitStructNode(StructNode *node) {
  if (node->GetProp() != SProp_TSEnum) {
    SetTypeId(node, TY_Class);
  }
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    TreeNode *t = node->GetField(i);
    (void) VisitClassField(t);
  }
  if (node->GetProp() == SProp_TSEnum) {
    TypeId tid = TY_None;
    unsigned tidx = 0;
    for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
      TreeNode *t = node->GetField(i);
      tid = MergeTypeId(tid, t->GetTypeId());
      tidx = MergeTypeIdx(tidx, t->GetTypeIdx());
    }
    if (tid == TY_None) {
      tid = TY_Int;
      tidx = (unsigned)TY_Int;
    }
    for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
      TreeNode *t = node->GetField(i);
      SetTypeId(t, tid);
      SetTypeIdx(t, tidx);
    }
    TreeNode *id = node->GetStructId();
    if (id) {
      SetTypeId(id, node->GetTypeId());
      SetTypeIdx(id, node->GetTypeIdx());
    }
  }
  (void) AstVisitor::VisitStructNode(node);
  return node;
}

LambdaNode *TypeInferVisitor::VisitLambdaNode(LambdaNode *node) {
  if (mFlags & FLG_trace_1) std::cout << "Visiting LambdaNode, id=" << node->GetNodeId() << "..." << std::endl;
  UpdateTypeId(node, TY_Function);
  (void) AstVisitor::VisitLambdaNode(node);
  return node;
}

LiteralNode *TypeInferVisitor::VisitLiteralNode(LiteralNode *node) {
  (void) AstVisitor::VisitLiteralNode(node);
  LitId id = node->GetData().mType;
  switch (id) {
    case LT_IntegerLiteral:
      SetTypeId(node, TY_Int);
      SetTypeIdx(node, TY_Int);
      break;
    case LT_FPLiteral:
      SetTypeId(node, TY_Float);
      SetTypeIdx(node, TY_Float);
      break;
    case LT_DoubleLiteral:
      SetTypeId(node, TY_Double);
      SetTypeIdx(node, TY_Double);
      break;
    case LT_StringLiteral:
      SetTypeId(node, TY_String);
      SetTypeIdx(node, TY_String);
      break;
    case LT_VoidLiteral:
      SetTypeId(node, TY_Undefined);
      break;
    default:
      break;
  }
  return node;
}

ReturnNode *TypeInferVisitor::VisitReturnNode(ReturnNode *node) {
  (void) AstVisitor::VisitReturnNode(node);
  TreeNode *res = node->GetResult();
  if (res) {
    UpdateTypeId(node, res->GetTypeId());
    UpdateTypeIdx(node, res->GetTypeIdx());
  }
  TreeNode *tn = mHandler->FindFunc(node);
  if (tn) {
    FunctionNode *func = static_cast<FunctionNode *>(tn);
    // use dummy PrimTypeNode as return type of function if not set to carry return TypeId
    if (!func->GetType()) {
      PrimTypeNode *type = mHandler->NewTreeNode<PrimTypeNode>();
      type->SetPrimType(TY_None);
      func->SetType(type);
    }
    UpdateFuncRetTypeId(func, node->GetTypeId(), node->GetTypeIdx());
    if (res) {
      // use res to update function's return type
      UpdateTypeUseNode(func->GetType(), res);
    }
  }
  return node;
}

StructLiteralNode *TypeInferVisitor::VisitStructLiteralNode(StructLiteralNode *node) {
  SetTypeId(node, TY_Class);
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    FieldLiteralNode *t = node->GetField(i);
    (void) VisitFieldLiteralNode(t);
  }
  (void) AstVisitor::VisitStructLiteralNode(node);
  return node;
}

TemplateLiteralNode *TypeInferVisitor::VisitTemplateLiteralNode(TemplateLiteralNode *node) {
  UpdateTypeId(node, TY_String);
  (void) AstVisitor::VisitTemplateLiteralNode(node);
  return node;
}

TerOperatorNode *TypeInferVisitor::VisitTerOperatorNode(TerOperatorNode *node) {
  TreeNode *ta = node->GetOpndA();
  TreeNode *tb = node->GetOpndB();
  TreeNode *tc = node->GetOpndC();
  (void) VisitTreeNode(ta);
  (void) VisitTreeNode(tb);
  (void) VisitTreeNode(tc);
  UpdateTypeId(node, tb);
  return node;
}

TypeOfNode *TypeInferVisitor::VisitTypeOfNode(TypeOfNode *node) {
  UpdateTypeId(node, TY_String);
  (void) AstVisitor::VisitTypeOfNode(node);
  return node;
}

TypeAliasNode *TypeInferVisitor::VisitTypeAliasNode(TypeAliasNode *node) {
  (void) AstVisitor::VisitTypeAliasNode(node);
  UserTypeNode *id = node->GetId();
  TreeNode *alias = node->GetAlias();
  UpdateTypeId(id, alias);
  return node;
}

UnaOperatorNode *TypeInferVisitor::VisitUnaOperatorNode(UnaOperatorNode *node) {
  (void) AstVisitor::VisitUnaOperatorNode(node);
  OprId op = node->GetOprId();
  TreeNode *ta = node->GetOpnd();
  switch (op) {
    case OPR_Plus:
    case OPR_Minus:
      UpdateTypeId(node, ta->GetTypeId());
      break;
    case OPR_PreInc:
    case OPR_Inc:
    case OPR_PreDec:
    case OPR_Dec:
      UpdateTypeId(ta, TY_Int);
      UpdateTypeId(node, TY_Int);
      break;
    case OPR_Bcomp:
      UpdateTypeId(ta, TY_Int);
      UpdateTypeId(node, TY_Int);
      break;
    case OPR_Not:
      UpdateTypeId(ta, TY_Boolean);
      UpdateTypeId(node, TY_Boolean);
      break;
    default: {
      NOTYETIMPL("VisitUnaOperatorNode()");
      break;
    }
  }
  return node;
}

UserTypeNode *TypeInferVisitor::VisitUserTypeNode(UserTypeNode *node) {
  (void) AstVisitor::VisitUserTypeNode(node);
  if (node->GetDims()) {
    SetTypeId(node, TY_Array);
    SetTypeIdx(node, TY_Array);
  } else if (node->GetId()) {
    UpdateTypeId(node, node->GetId());
    UpdateTypeIdx(node, node->GetId());
  }
  TreeNode *parent = node->GetParent();
  if (parent && parent->IsIdentifier()) {
    // typeid: merge -> user
    if (parent->IsTypeIdMerge()) {
      SetTypeId(parent, TY_User);
      SetUpdated();
    } else if (parent->IsTypeIdArray()) {
      TreeNode *idnode = node->GetId();
      if (idnode && idnode->IsIdentifier()) {
        // number, string could have been used
        // as usertype identifier instand of primtype by parser
        IdentifierNode *id = static_cast<IdentifierNode *>(idnode);
        TypeId tid = TY_None;
        unsigned stridx = id->GetStrIdx();
        if (stridx == gStringPool.GetStrIdx("number")) {
          tid = TY_Number;
        } else if (stridx == gStringPool.GetStrIdx("string")) {
          tid = TY_String;
        }
        if (tid != TY_None) {
          PrimArrayTypeNode *type = mHandler->NewTreeNode<PrimArrayTypeNode>();
          PrimTypeNode *pt = mHandler->NewTreeNode<PrimTypeNode>();
          pt->SetPrimType(tid);
          type->SetPrim(pt);
          type->SetDims(node->GetDims());
          IdentifierNode *parentid = static_cast<IdentifierNode *>(parent);
          parentid->SetType(type);

          // clean up parent info
          id->SetParent(NULL);
          node->SetParent(NULL);

          // set updated
          SetUpdated();
        }
      }
    }
  }
  return node;
}

UserTypeNode *ShareUTVisitor::VisitUserTypeNode(UserTypeNode *node) {
  // skip it
  return node;

  (void) AstVisitor::VisitUserTypeNode(node);

  // skip for array
  if (node->GetDims()) {
    return node;
  }

  TreeNode *idnode = node->GetId();
  if (idnode && idnode->IsIdentifier()) {
    IdentifierNode *id = static_cast<IdentifierNode *>(idnode);
    ASTScope *scope = id->GetScope();
    TreeNode *type = scope->FindTypeOf(id->GetStrIdx());
    if (type && type != node && type->IsUserType()) {
      UserTypeNode *ut = static_cast<UserTypeNode *>(type);
      if (node->GetType() == ut->GetType()) {
        // do not share if there are generics
        if (ut->GetTypeGenericsNum() == 0) {
          return ut;
        }
      }
    }
  }
  return node;
}

bool CheckTypeVisitor::IsCompatible(TypeId tid, TypeId target) {
  if (tid == target) {
    return true;
  }

  bool result = false;
  switch (target) {
    case TY_Number: {
      switch (tid) {
        case TY_None:
        case TY_Int:
        case TY_Long:
        case TY_Float:
        case TY_Double:
          result = true;
          break;
        default:
          result = false;
          break;
      }
      break;
    }
    case TY_Any:
      // TY_Any or unspecified matches everything
      result = true;
      break;
    default:
      // ok if same typeid
      result = (target == tid);
      break;
  }

  // tid being TY_None means untouched
  result = result || (tid == TY_None);

  return result;
}

IdentifierNode *CheckTypeVisitor::VisitIdentifierNode(IdentifierNode *node) {
  (void) AstVisitor::VisitIdentifierNode(node);

  TreeNode *d = mHandler->FindDecl(node);
  if (d && d->IsDecl()) {
    DeclNode *decl = static_cast<DeclNode *>(d);
    TreeNode *var = decl->GetVar();
    if (var && var != node && var->IsIdentifier()) {
      IdentifierNode *id = static_cast<IdentifierNode *>(var);
      bool result = false;
      TreeNode *type = id->GetType();
      if (type) {
        TypeId id = node->GetTypeId();
        TypeId target = TY_None;
        switch (type->GetKind()) {
          case NK_PrimType: {
            PrimTypeNode *ptn = static_cast<PrimTypeNode *>(type);
            target = ptn->GetPrimType();
            break;
          }
          case NK_UserType: {
            target = var->GetTypeId();
            break;
          }
          default: {
            target = var->GetTypeId();
            break;
          }
        }
        result = IsCompatible(id, target);
        if (!result || (mFlags & FLG_trace_3)) {
          std::cout << " Type Compatiblity : " << result << " : "
                    << AstDump::GetEnumTypeId(target) << " "
                    << AstDump::GetEnumTypeId(id) << std::endl;
        }
      }
    }
  }
  return node;
}

IdentifierNode *ChangeTypeIdxVisitor::VisitIdentifierNode(IdentifierNode *node) {
  (void) AstVisitor::VisitIdentifierNode(node);
  if (node->GetStrIdx() == mStrIdx) {
    mHandler->GetUtil()->SetTypeIdx(node, mTypeIdx);
  }
  return node;
}

}
