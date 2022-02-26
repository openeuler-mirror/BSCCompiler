#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.

######################################################################
#                        Expression                                  #
######################################################################

rule PrimaryExpression : ONEOF(
  Literal,
  Identifier
)

rule DimExprs : DimExpr + ZEROORMORE(DimExpr)

rule DimExpr :  '[' + Expression + ']'

rule Expression : ONEOF(
  PrimaryExpression,
  UnaryExpression)

rule UnaryExpression : ONEOF(
  PreIncrementExpression,
  PreDecrementExpression,
  PostIncrementExpression,
  PostDecrementExpression)

rule PreIncrementExpression : "++" + PrimaryExpression
  attr.action : BuildUnaryOperation(%1, %2)

rule PreDecrementExpression : "--" + PrimaryExpression
  attr.action : BuildUnaryOperation(%1, %2)

rule PostIncrementExpression : PrimaryExpression + "++"
  attr.action : BuildPostfixOperation(%2, %1)

rule PostDecrementExpression : PrimaryExpression + "--"
  attr.action : BuildPostfixOperation(%2, %1)

######################################################################
#                         Variable                                   #
######################################################################

rule GlobalVariableDeclarationStatement : VariableDeclaration + ';'
  attr.property : Top

rule LocalVariableDeclarationStatement : VariableDeclaration + ';'

rule VariableDeclaration : ZEROORMORE(VariableModifier) + Type + VariableDeclaratorList
  attr.action: BuildDecl(%2, %3)
  attr.action: AddModifier(%1)

rule VariableModifier : ONEOF(
  "static",
  "const",
  "volatile",
  "restrict")

rule VariableDeclaratorList : VariableDeclarator + ZEROORMORE(',' + VariableDeclarator)
  attr.action: BuildVarList(%1, %2)

rule VariableDeclarator : VariableDeclaratorId + ZEROORONE('=' + VariableInitializer)
  attr.action: AddInitTo(%1, %2)

rule VariableDeclaratorId : Identifier + ZEROORONE(Dims)
  attr.action: AddDimsTo(%1, %2)

rule VariableInitializer : ONEOF(
  Expression,
  ArrayInitializer)

rule ArrayInitializer : '{' + ZEROORONE(VariableInitializerList) + ZEROORONE(',') + '}'

rule VariableInitializerList: VariableInitializer + ZEROORMORE(',' + VariableInitializer)

rule Dims : Dim + ZEROORMORE(Dim)
 attr.action: BuildDims(%1, %2)

rule Dim  :  '[' + ']'
 attr.action: BuildDim(%1)

######################################################################
#                         statement                                  #
######################################################################

rule Statement : ONEOF(LocalVariableDeclarationStatement,
                       ExpressionStatement,
                       ReturnStatement)
  attr.property: Single

rule ExpressionStatement : StatementExpression + ';'

rule StatementExpression : ONEOF(
  PreIncrementExpression,
  PreDecrementExpression,
  PostIncrementExpression,
  PostDecrementExpression,
  )
  attr.property: Single

rule ReturnStatement : "return" + ZEROORONE(Expression) + ';'
  attr.action : BuildReturn(%2)


######################################################################
#                         Function                                   #
######################################################################

rule GlobalFuncDeclaration : FuncDeclaration
  attr.property : Top

rule FuncDeclaration : ZEROORMORE(FuncModifier) + FuncHeader + FuncBody
  attr.action: AddModifierTo(%2, %1)
  attr.action: AddFunctionBodyTo(%2, %3)

rule FuncBody        : ONEOF(Block, ';')
  attr.property : Single

rule FuncHeader      : ONEOF(Result + FuncDeclarator)
  attr.action.%1: AddType(%2, %1)
  attr.property : Single

rule Result            : ONEOF(Type, "void")
  attr.property : Single

rule FuncDeclarator  : Identifier + '(' + ZEROORONE(FormalParameters) + ')'
  attr.action: BuildFunction(%1)
  attr.action: AddParams(%3)

rule FuncAttr : ONEOF("const", "static")
  attr.property : Single

rule FuncModifier : ONEOF(FuncAttr)
  attr.property : Single

rule FormalParameters  : ONEOF(FormalParameter + ZEROORMORE(',' + FormalParameter))
  attr.property : Single

rule FormalParameter   : ZEROORMORE(VariableModifier) + Type + VariableDeclaratorId
  attr.action: BuildDecl(%2, %3)
  attr.action: AddModifier(%1)

######################################################################
#                        Block                                       #
######################################################################

rule BlockStatement  : ONEOF(LocalVariableDeclarationStatement, Statement)
  attr.property : Single

rule BlockStatements : BlockStatement + ZEROORMORE(BlockStatement)

rule Block           : '{' + ZEROORONE(BlockStatements) + '}'
  attr.action: BuildBlock(%2)
