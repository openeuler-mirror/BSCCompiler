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
#include <queue>
#include <cstring>
#include "ast_cfg.h"
#include "ast_dfa.h"
#include "ast_handler.h"
#include "gen_astdump.h"

namespace maplefe {

class CollectNestedFuncs : public AstVisitor {
  private:
    CfgBuilder   *mBuilder;
    AST_Function *mFunc;

  public:
    CollectNestedFuncs(CfgBuilder *b, AST_Function *f) : mBuilder(b), mFunc(f) {}

    FunctionNode *VisitFunctionNode(FunctionNode *node) {
      if(TreeNode *body = node->GetBody())
        HandleNestedFunction(node, body);
      return node;
    }

    LambdaNode *VisitLambdaNode(LambdaNode *node) {
      if(TreeNode *body = node->GetBody())
        HandleNestedFunction(node, body);
      return node;
    }

    void HandleNestedFunction(TreeNode *func, TreeNode *body) {
        AST_Function *current = mFunc;
        mFunc = mBuilder->NewFunction(func);
        current->AddNestedFunction(mFunc);
        AstVisitor::VisitTreeNode(body);
        mFunc = current;
    }
};

// Create AST_Function nodes for a module
AST_Function *CfgBuilder::InitAstFunctions(ModuleNode *module) {
  AST_Function *module_func = NewFunction(module);
  CollectNestedFuncs collector(this, module_func);
  collector.Visit(module);
  return module_func;
}

// Initialize a AST_Function node
void CfgBuilder::InitializeFunction(AST_Function *func) {
  // Create the entry BB and exit BB of current function
  AST_BB *entry = NewBB(BK_Uncond);
  func->SetEntryBB(entry);
  AST_BB *exit = NewBB(BK_Join);
  func->SetExitBB(exit);
  CfgBuilder::Push(mThrowBBs, exit, nullptr);
  // Initialize the working function and BB
  mCurrentFunction = func;
  mCurrentBB = NewBB(BK_Uncond);
  entry->AddSuccessor(mCurrentBB);
}

// Finalize a AST_Function node
void CfgBuilder::FinalizeFunction() {
  CfgBuilder::Pop(mThrowBBs);
  AST_BB *exit = mCurrentFunction->GetExitBB();
  mCurrentBB->AddSuccessor(exit);
  mCurrentFunction->SetLastBBId(AST_BB::GetLastId());
  mCurrentFunction = nullptr;
  mCurrentBB = nullptr;
}

// Push a BB to target BB stack
void CfgBuilder::Push(TargetBBStack &stack, AST_BB* bb, TreeNode *label) {
  unsigned idx = 0;
  if(label && label->GetKind() == NK_Identifier)
    idx = static_cast<IdentifierNode *>(label)->GetStrIdx();
  stack.push_back(TargetBB{bb, idx});
}

// Look up a target BB
AST_BB *CfgBuilder::LookUp(TargetBBStack &stack, TreeNode *label) {
  unsigned idx = 0;
  if(label && label->GetKind() == NK_Identifier)
    idx = static_cast<IdentifierNode *>(label)->GetStrIdx();
  if(idx == 0)
    return stack.back().first;
  for(auto it = stack.rbegin(); it != stack.rend(); ++it)
    if(it->second && it->second == idx)
      return it->first;
  MASSERT(0 && "Unexpected: Target not found.");
  return nullptr;
}

// Pop from a target BB stack
void CfgBuilder::Pop(TargetBBStack &stack) {
  stack.pop_back();
}

// Handle a function
FunctionNode *CfgBuilder::VisitFunctionNode(FunctionNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// Handle a lambda
LambdaNode *CfgBuilder::VisitLambdaNode(LambdaNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For control flow
ReturnNode *CfgBuilder::VisitReturnNode(ReturnNode *node) {
  mCurrentBB->AddStatement(node);
  AST_BB *exit = mCurrentFunction->GetExitBB();
  mCurrentBB->AddSuccessor(exit);
  mCurrentBB->SetKind(BK_Terminated);
  mCurrentBB->SetAttr(AK_Return);
  return node;
}

// For control flow
CondBranchNode *CfgBuilder::VisitCondBranchNode(CondBranchNode *node) {
  mCurrentBB->SetKind(BK_Branch);
  //mCurrentBB->AddStatement(node);
  mCurrentBB->SetAuxNode(node);

  TreeNode *cond = node->GetCond();
  // Set predicate of current BB
  mCurrentBB->SetPredicate(cond);
  // Add a Br stmt to current BB
  BrNode *brn = (BrNode*)gTreePool.NewTreeNode(sizeof(BrNode));
  new (brn) BrNode();
  brn->SetCond(cond);
  mCurrentBB->AddStatement(brn);

  // Save current BB
  AST_BB *current_bb = mCurrentBB;

  // Create a new BB for true branch
  mCurrentBB = NewBB(BK_Uncond);
  current_bb->AddSuccessor(mCurrentBB);

  // Visit true branch first
  VisitTreeNode(node->GetTrueBranch());

  // Create a BB for the join point
  AST_BB *join = NewBB(BK_Join);
  mCurrentBB->AddSuccessor(join);

  TreeNode *false_branch = node->GetFalseBranch();
  if(false_branch == nullptr) {
    current_bb->AddSuccessor(join);
  } else {
    mCurrentBB = NewBB(BK_Uncond);
    current_bb->AddSuccessor(mCurrentBB);
    // Visit false branch if it exists
    VisitTreeNode(false_branch);
    mCurrentBB->AddSuccessor(join);
  }
  // Keep going with the BB at the join point
  mCurrentBB = join;
  return node;
}

// For control flow
// ForLoopProp: FLP_Regular, FLP_JSIn, FLP_JSOf
ForLoopNode *CfgBuilder::VisitForLoopNode(ForLoopNode *node) {
  // Visit all inits
  for (unsigned i = 0; i < node->GetInitsNum(); ++i) {
    VisitTreeNode(node->GetInitAtIndex(i));
  }

  AST_BB *current_bb = mCurrentBB;
  // Create a new BB for loop header
  mCurrentBB = NewBB(BK_LoopHeader);

  // Add current node to the loop header BB
  //mCurrentBB->AddStatement(node);
  mCurrentBB->SetAuxNode(node);
  current_bb->AddSuccessor(mCurrentBB);
  // Set current_bb to be loop header
  current_bb = mCurrentBB;

  if(node->GetProp() == FLP_Regular) {
    TreeNode *cond = node->GetCond();
    // Set predicate of current BB
    mCurrentBB->SetPredicate(cond);
    // Add a Br stmt to current BB
    BrNode *brn = (BrNode*)gTreePool.NewTreeNode(sizeof(BrNode));
    new (brn) BrNode();
    brn->SetCond(cond);
    mCurrentBB->AddStatement(brn);
  } else
    // Set predicate to be current ForLoopNode when it is FLP_JSIn or FLP_JSOf
    mCurrentBB->SetPredicate(node);

  // Create a BB for loop body
  mCurrentBB = NewBB(BK_Uncond);
  current_bb->AddSuccessor(mCurrentBB);
  // Create a new BB for getting out of the loop
  AST_BB *loop_exit = NewBB(BK_Join);

  // Push loop_exit and current_bb to stacks for 'break' and 'continue'
  CfgBuilder::Push(mBreakBBs, loop_exit, node->GetLabel());
  CfgBuilder::Push(mContinueBBs, current_bb, node->GetLabel());
  // Visit loop body
  VisitTreeNode(node->GetBody());
  CfgBuilder::Pop(mContinueBBs);
  CfgBuilder::Pop(mBreakBBs);

  // Visit all updates
  for (unsigned i = 0; i < node->GetUpdatesNum(); ++i) {
    VisitTreeNode(node->GetUpdateAtIndex(i));
  }
  // Add a back edge to loop header
  mCurrentBB->AddSuccessor(current_bb);
  current_bb->AddSuccessor(loop_exit);
  mCurrentBB = loop_exit;
  return node;
}

// For control flow
WhileLoopNode *CfgBuilder::VisitWhileLoopNode(WhileLoopNode *node) {
  AST_BB *current_bb = mCurrentBB;
  // Create a new BB for loop header
  mCurrentBB = NewBB(BK_LoopHeader);
  // Add current node to the loop header BB
  //mCurrentBB->AddStatement(node);
  mCurrentBB->SetAuxNode(node);
  current_bb->AddSuccessor(mCurrentBB);
  // Set current_bb to be loop header
  current_bb = mCurrentBB;

  TreeNode *cond = node->GetCond();
  // Set predicate of current BB
  mCurrentBB->SetPredicate(cond);
  // Add a Br stmt to current BB
  BrNode *brn = (BrNode*)gTreePool.NewTreeNode(sizeof(BrNode));
  new (brn) BrNode();
  brn->SetCond(cond);
  mCurrentBB->AddStatement(brn);

  // Create a BB for loop body
  mCurrentBB = NewBB(BK_Uncond);
  current_bb->AddSuccessor(mCurrentBB);
  // Create a new BB for getting out of the loop
  AST_BB *loop_exit = NewBB(BK_Join);

  // Push loop_exit and current_bb to stacks for 'break' and 'continue'
  CfgBuilder::Push(mBreakBBs, loop_exit, node->GetLabel());
  CfgBuilder::Push(mContinueBBs, current_bb, node->GetLabel());
  // Visit loop body
  VisitTreeNode(node->GetBody());
  CfgBuilder::Pop(mContinueBBs);
  CfgBuilder::Pop(mBreakBBs);

  // Add a back edge to loop header
  mCurrentBB->AddSuccessor(current_bb);
  current_bb->AddSuccessor(loop_exit);
  mCurrentBB = loop_exit;
  return node;
}

// For control flow
DoLoopNode *CfgBuilder::VisitDoLoopNode(DoLoopNode *node) {
  AST_BB *current_bb = mCurrentBB;
  // Create a new BB for loop header
  mCurrentBB = NewBB(BK_LoopHeader);
  // Add current node to the loop header BB
  //mCurrentBB->AddStatement(node);
  mCurrentBB->SetAuxNode(node);
  current_bb->AddSuccessor(mCurrentBB);
  // Set current_bb to be loop header
  current_bb = mCurrentBB;

  // Create a BB for loop body
  mCurrentBB = NewBB(BK_Uncond);
  current_bb->AddSuccessor(mCurrentBB);
  // Create a new BB for getting out of the loop
  AST_BB *loop_exit = NewBB(BK_Join);

  // Push loop_exit and current_bb to stacks for 'break' and 'continue'
  CfgBuilder::Push(mBreakBBs, loop_exit, node->GetLabel());
  CfgBuilder::Push(mContinueBBs, current_bb, node->GetLabel());
  // Visit loop body
  VisitTreeNode(node->GetBody());
  CfgBuilder::Pop(mContinueBBs);
  CfgBuilder::Pop(mBreakBBs);

  TreeNode *cond = node->GetCond();
  // Set predicate of current BB
  mCurrentBB->SetPredicate(cond);
  // Add a Br stmt to current BB
  BrNode *brn = (BrNode*)gTreePool.NewTreeNode(sizeof(BrNode));
  new (brn) BrNode();
  brn->SetCond(cond);
  mCurrentBB->AddStatement(brn);

  // Add a back edge to loop header
  mCurrentBB->AddSuccessor(current_bb);
  mCurrentBB->AddSuccessor(loop_exit);
  mCurrentBB = loop_exit;
  return node;
}

// For control flow
ContinueNode *CfgBuilder::VisitContinueNode(ContinueNode *node) {
  mCurrentBB->AddStatement(node);
  // Get the loop header
  AST_BB *loop_header = CfgBuilder::LookUp(mContinueBBs, node->GetTarget());
  // Add the loop header as a successor of current BB
  mCurrentBB->AddSuccessor(loop_header);
  mCurrentBB->SetKind(BK_Terminated);
  mCurrentBB->SetAttr(AK_Cont);
  return node;
}

// For control flow
BreakNode *CfgBuilder::VisitBreakNode(BreakNode *node) {
  mCurrentBB->AddStatement(node);
  // Get the target BB for a loop or switch statement
  AST_BB *exit = CfgBuilder::LookUp(mBreakBBs, node->GetTarget());
  // Add the target as a successor of current BB
  mCurrentBB->AddSuccessor(exit);
  mCurrentBB->SetKind(BK_Terminated);
  mCurrentBB->SetAttr(AK_Break);
  return node;
}

// For control flow
SwitchNode *CfgBuilder::VisitSwitchNode(SwitchNode *node) {
  mCurrentBB->SetKind(BK_Switch);
  mCurrentBB->AddStatement(node);
  // Set the root node of current BB
  mCurrentBB->SetAuxNode(node);

  // Save current BB
  AST_BB *current_bb = mCurrentBB;

  // Create a new BB for getting out of the switch block
  AST_BB *exit = NewBB(BK_Join);
  CfgBuilder::Push(mBreakBBs, exit, nullptr);
  AST_BB *prev_block = nullptr;
  TreeNode *switch_expr = node->GetExpr();
  for (unsigned i = 0; i < node->GetCasesNum(); ++i) {
    AST_BB *case_bb = NewBB(BK_Case);
    current_bb->AddSuccessor(case_bb);

    TreeNode *case_node = node->GetCaseAtIndex(i);
    // Set the auxiliary node and predicate for current case BB
    case_bb->SetAuxNode(case_node);
    case_bb->SetPredicate(switch_expr);
    // Add a Br stmt to current BB
    BrNode *brn = (BrNode*)gTreePool.NewTreeNode(sizeof(BrNode));
    new (brn) BrNode();
    brn->SetCond(switch_expr);
    mCurrentBB->AddStatement(brn);

    bool is_default = false;
    TreeNode *case_expr = nullptr;
    if(case_node->GetKind() == NK_SwitchCase) {
      // Use the first label node of current SwitchCaseNode
      TreeNode *label_node = static_cast<SwitchCaseNode *>(case_node)->GetLabelAtIndex(0);
      if(label_node->GetKind() == NK_SwitchLabel) {
        is_default = static_cast<SwitchLabelNode *>(label_node)->IsDefault();
        case_expr = static_cast<SwitchLabelNode *>(label_node)->GetValue();
      }
    }

    // Optimize for default case
    if(is_default) {
      mCurrentBB = case_bb;
      case_bb->SetKind(BK_Uncond);
    } else {
      mCurrentBB = NewBB(BK_Uncond);
      case_bb->AddSuccessor(mCurrentBB);
    }

    // Add a fall-through edge if needed
    if(prev_block) {
      prev_block->AddSuccessor(mCurrentBB);
    }

    // Visit all statements of current case
    if(case_node->GetKind() == NK_SwitchCase) {
      SwitchCaseNode *cnode = static_cast<SwitchCaseNode *>(case_node);
      for (unsigned i = 0; i < cnode->GetStmtsNum(); ++i) {
        if (auto t = cnode->GetStmtAtIndex(i))
          VisitTreeNode(t);
      }
    }

    // Prepare for next case
    prev_block = mCurrentBB;
    current_bb = case_bb;
  }
  CfgBuilder::Pop(mBreakBBs);

  // Connect to the exit BB of this switch statement
  prev_block->AddSuccessor(exit);
  if(prev_block != current_bb) {
    current_bb->AddSuccessor(exit);
  }
  mCurrentBB = exit;
  return node;
}

// For control flow
TryNode *CfgBuilder::VisitTryNode(TryNode *node) {
  mCurrentBB->SetKind(BK_Try);
  //mCurrentBB->AddStatement(node);
  mCurrentBB->SetAuxNode(node);

  auto try_block_node = node->GetBlock();
  //mCurrentBB->AddStatement(try_block_node);

  unsigned num = node->GetCatchesNum();
  AST_BB *catch_bb = num ? NewBB(BK_Catch) : nullptr;

  auto finally_node = node->GetFinally();
  // Create a BB for the join point
  AST_BB *join = finally_node ? NewBB(BK_Finally) : NewBB(BK_Join);

  // Save current BB
  AST_BB *current_bb = mCurrentBB;
  // Create a new BB for current block node
  mCurrentBB = NewBB(BK_Uncond);
  current_bb->AddSuccessor(mCurrentBB);

  // Add an edge for exception
  current_bb->AddSuccessor(num ? catch_bb : join);

  // Visit try block
  if(num) {
    CfgBuilder::Push(mThrowBBs, catch_bb, nullptr);
    AstVisitor::VisitBlockNode(try_block_node);
    CfgBuilder::Pop(mThrowBBs);
  } else
    AstVisitor::VisitBlockNode(try_block_node);

  // Add an edge to join point
  mCurrentBB->AddSuccessor(join);

  // JavaScript can have one catch block, or one finally block without catch block
  // Other languages, such as C++ and Java, may have multiple catch blocks
  AST_BB *curr_bb = mCurrentBB;
  for (unsigned i = 0; i < num; ++i) {
    if(i > 0)
      catch_bb = NewBB(BK_Catch);
    // Add an edge to catch bb
    curr_bb->AddSuccessor(catch_bb);
    mCurrentBB = catch_bb;
    auto catch_node = node->GetCatchAtIndex(i);
    catch_bb->AddStatement(catch_node);

    auto catch_block = catch_node->GetBlock();
    catch_bb->AddStatement(catch_block);
    AstVisitor::VisitBlockNode(catch_block);

    mCurrentBB->AddSuccessor(join);
    curr_bb = catch_bb;
  }

  mCurrentBB = join;
  if(finally_node) {
    // For finally block
    //mCurrentBB->AddStatement(finally_node);
    mCurrentBB->SetAuxNode(finally_node);
    AstVisitor::VisitTreeNode(finally_node);
    curr_bb = NewBB(BK_Join);
    mCurrentBB->AddSuccessor(curr_bb);
    // Add an edge to recent catch BB or exit BB
    mCurrentBB->AddSuccessor(CfgBuilder::LookUp(mThrowBBs, nullptr));
    mCurrentBB = curr_bb;
  }
  return node;
}

// For control flow
ThrowNode *CfgBuilder::VisitThrowNode(ThrowNode *node) {
  mCurrentBB->AddStatement(node);
  // Get the catch/exit bb for this throw statement
  AST_BB *catch_bb = CfgBuilder::LookUp(mThrowBBs, nullptr);
  // Add the loop header as a successor of current BB
  mCurrentBB->AddSuccessor(catch_bb);
  mCurrentBB->SetKind(BK_Terminated);
  mCurrentBB->SetAttr(AK_Throw);
  return node;
}

// For control flow
BlockNode *CfgBuilder::VisitBlockNode(BlockNode *node) {
  //mCurrentBB->AddStatement(node);
  mCurrentBB->SetAuxNode(node);
  // Check if current block constains any JS_Let or JS_Const DeclNode
  unsigned i, num = node->GetChildrenNum();
  for (i = 0; i < num; ++i) {
    TreeNode *child = node->GetChildAtIndex(i);
    if(child == nullptr || child->GetKind() != NK_Decl) {
      continue;
    }
    DeclNode *decl = static_cast<DeclNode *>(child);
    if(decl->GetProp() == JS_Let || decl->GetProp() == JS_Const) {
      break;
    }
  }
  if(i >= num) {
    // Do not create BB for current block when no JS_Let or JS_Const DeclNode inside
    // Visit all child nodes
    AstVisitor::VisitBlockNode(node);
  } else {
    mCurrentBB->SetKind(BK_Block);
    // Set the auxiliary node of this BB
    mCurrentBB->SetAuxNode(node);

    // Create a BB for the join point
    AST_BB *join = NewBB(BK_Join);

    // Needs BBs for current block
    // Save current BB
    AST_BB *current_bb = mCurrentBB;
    // Create a new BB for current block node
    mCurrentBB = NewBB(BK_Uncond);
    current_bb->AddSuccessor(mCurrentBB);

    // Visit all child nodes
    AstVisitor::VisitBlockNode(node);

    mCurrentBB->AddSuccessor(join);
    // This edge is to determine the block range for JS_Let or JS_Const DeclNode
    //current_bb->AddSuccessor(join);

    mCurrentBB = join;
  }
  return node;
}

// For PassNode
PassNode *CfgBuilder::VisitPassNode(PassNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
TemplateLiteralNode *CfgBuilder::VisitTemplateLiteralNode(TemplateLiteralNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
ImportNode *CfgBuilder::VisitImportNode(ImportNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
ExportNode *CfgBuilder::VisitExportNode(ExportNode *node) {
  mCurrentBB->AddStatement(node);
  AstVisitor::VisitExportNode(node);
  return node;
}

// For statement of current BB
DeclNode *CfgBuilder::VisitDeclNode(DeclNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
ParenthesisNode *CfgBuilder::VisitParenthesisNode(ParenthesisNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
CastNode *CfgBuilder::VisitCastNode(CastNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
ArrayElementNode *CfgBuilder::VisitArrayElementNode(ArrayElementNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
VarListNode *CfgBuilder::VisitVarListNode(VarListNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
ExprListNode *CfgBuilder::VisitExprListNode(ExprListNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
UnaOperatorNode *CfgBuilder::VisitUnaOperatorNode(UnaOperatorNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
BinOperatorNode *CfgBuilder::VisitBinOperatorNode(BinOperatorNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
TerOperatorNode *CfgBuilder::VisitTerOperatorNode(TerOperatorNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
InstanceOfNode *CfgBuilder::VisitInstanceOfNode(InstanceOfNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
TypeOfNode *CfgBuilder::VisitTypeOfNode(TypeOfNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
NewNode *CfgBuilder::VisitNewNode(NewNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
DeleteNode *CfgBuilder::VisitDeleteNode(DeleteNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
CallNode *CfgBuilder::VisitCallNode(CallNode *node) {
  mCurrentBB->AddStatement(node);
  mCurrentBB->SetAttr(AK_HasCall);
  return node;
}

// For statement of current BB
AssertNode *CfgBuilder::VisitAssertNode(AssertNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
UserTypeNode *CfgBuilder::VisitUserTypeNode(UserTypeNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
IdentifierNode *CfgBuilder::VisitIdentifierNode(IdentifierNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
LiteralNode *CfgBuilder::VisitLiteralNode(LiteralNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// For statement of current BB
StructNode *CfgBuilder::VisitStructNode(StructNode *node) {
  mCurrentBB->AddStatement(node);
  return node;
}

// Allocate a new AST_Function node
AST_Function *CfgBuilder::NewFunction(TreeNode *node)   {
  AST_Function *func = new(mHandler->GetMemPool()->Alloc(sizeof(AST_Function))) AST_Function;
  func->SetFunction(node);
  return func;
}

// Allocate a new AST_BB node
AST_BB *CfgBuilder::NewBB(BBKind k) {
  return new(mHandler->GetMemPool()->Alloc(sizeof(AST_BB))) AST_BB(k);
}

// Helper for a node in dot graph
static std::string BBLabelStr(AST_BB *bb, const char *shape = nullptr, const char *fn = nullptr) {
  static const char* const kBBNames[] =
  { "unknown", "uncond", "block", "branch", "loop", "switch", "case", "try", "catch", "finally",
    "yield", "term", "join" };
  if(shape == nullptr)
    return kBBNames[bb->GetKind()];
  std::string str("BB" + std::to_string(bb->GetId()));
  str += " [label=\"" + str + (shape[0] == 'e' ? std::string("\\n") + kBBNames[bb->GetKind()] : "")
    + (fn ? std::string("\\n\\\"") + fn + "\\\"" : "") + "\", shape=" + shape + "];\n";
  return str;
}

// Dump current AST_Function node
void AST_Function::Dump() {
  const char *func_name = GetName();
  std::cout << "Function " << func_name  << " {" << std::endl;
  unsigned num = GetNestedFunctionsNum();
  if(num > 0) {
    std::cout << "Nested Functions: " << num << " [" << std::endl;
    for(unsigned i = 0; i < num; ++i) {
      AST_Function *afunc = GetNestedFunctionAtIndex(i);
      const char *fname = afunc->GetName();
      std::cout << "Function: " << i + 1 << " " << fname << std::endl;
      afunc->Dump();
    }
    std::cout << "] // Nested Functions" << std::endl;
  }
  std::cout << "BBs: [" << std::endl;

  std::stack<AST_BB*> bb_stack;
  AST_BB *entry = GetEntryBB(), *exit = GetExitBB();
  bb_stack.push(exit);
  bb_stack.push(entry);
  std::set<AST_BB*> visited;
  visited.insert(exit);
  visited.insert(entry);
  // Dump CFG in dot format
  std::string dot("---\ndigraph CFG_");
  dot = dot + func_name + " {\n" + BBLabelStr(entry, "box", func_name) + BBLabelStr(exit, "doubleoctagon");
  const char* scoped = " [style=dashed color=grey];";
  while(!bb_stack.empty()) {
    AST_BB *bb = bb_stack.top();
    bb_stack.pop();
    unsigned succ_num = bb->GetSuccessorsNum();
    std::cout << "BB" << bb->GetId() << ", " << BBLabelStr(bb) << (succ_num ? " ( succ: " : " ( Exit ");
    for(unsigned i = 0; i < succ_num; ++i) {
      AST_BB *curr = bb->GetSuccessorAtIndex(i);
      std::cout << "BB" << curr->GetId() << " ";
      dot += "BB" + std::to_string(bb->GetId()) + " -> BB" + std::to_string(curr->GetId())
        + (bb == entry ? (curr == exit ? scoped : ";") :
            succ_num == 1 ? ";" : i ? bb->GetKind() == BK_Block ? scoped :
            " [color=darkred];" : " [color=darkgreen];") + "\n";
      if(visited.find(curr) == visited.end()) {
        bb_stack.push(curr);
        visited.insert(curr);
        dot += BBLabelStr(curr, "ellipse");
      }
    }
    std::cout << ")" << std::endl;
    unsigned stmt_num = bb->GetStatementsNum();
    if(stmt_num) {
      for(unsigned i = 0; i < stmt_num; ++i) {
        TreeNode *stmt = bb->GetStatementAtIndex(i);
        std::cout << "  " << i + 1 << ". NodeId: " << stmt->GetNodeId() << ", "
          << AstDump::GetEnumNodeKind(stmt->GetKind()) << " : ";
        stmt->Dump(0);
        std::cout  << std::endl;
      }
    }
  }
  std::cout << dot << "} // CFG in dot format" << std::endl;
  std::cout << "] // BBs\nLastBBId" << (num ? " (Including nested functions)" : "") << ": "
    << GetLastBBId() << "\n} // Function" << std::endl;
}

void AST_CFG::Build() {
  if (mTrace) std::cout << "============== BuildCFG ==============" << std::endl;
  CfgBuilder builder(mHandler, mTrace, true);

  ModuleNode *module = mHandler->GetASTModule();

  // Set the init function for current module
  AST_Function *func = builder.InitAstFunctions(module);

  mHandler->SetFunction(func);

  std::queue<AST_Function*> funcQueue;
  funcQueue.push(func);
  while(!funcQueue.empty()) {
    func = funcQueue.front();
    funcQueue.pop();
    // Start to build CFG for current function
    builder.InitializeFunction(func);
    TreeNode *node = func->GetFunction();
    switch(node->GetKind()) {
      case NK_Function:
        builder.Visit(static_cast<FunctionNode*>(node)->GetBody());;
        break;
      case NK_Lambda:
        builder.Visit(static_cast<LambdaNode*>(node)->GetBody());;
        break;
      default:
        builder.Visit(node);
    }
    builder.FinalizeFunction();
    for(unsigned i = 0; i < func->GetNestedFunctionsNum(); ++i)
      funcQueue.push(func->GetNestedFunctionAtIndex(i));
  }
}

}
