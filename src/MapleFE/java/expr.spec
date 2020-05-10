# Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
#
# OpenArkFE is licensed under the Mulan PSL v1.
# You can use this software according to the terms and conditions of the Mulan PSL v1.
# You may obtain a copy of Mulan PSL v1 at:
#
#  http://license.coscl.org.cn/MulanPSL
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v1 for more details.
#
###################################################################################
# This file defines the Java Expr
# https://docs.oracle.com/javase/specs/jls/se12/html/jls-15.html
###################################################################################

# I just put all TO-DO rules here
rule Class    : "class"

rule ArrayInitializer : "arrayinitializer"

rule PackageName : ONEOF(Identifier,
                         PackageName + '.' + Identifier)
rule TypeName    : ONEOF(Identifier,
                         PackageOrTypeName + '.' + Identifier)
rule PackageOrTypeName : ONEOF(Identifier,
                               PackageOrTypeName + '.' + Identifier)
rule ExpressionName : ONEOF(Identifier,
                            AmbiguousName + '.' + Identifier)
rule MethodName     : Identifier
rule AmbiguousName  : ONEOF(Identifier,
                            AmbiguousName + '.' + Identifier)

rule ClassLiteral : ONEOF(
  TypeName + ZEROORMORE('[' + ']') + '.' + Class,
  NumericType + ZEROORMORE('[' + ']') + '.' + Class,
  "boolean" + ZEROORMORE('[' + ']') + '.' + Class,
  "void" + '.' + Class)

rule PrimaryNoNewArray : ONEOF(
  Literal,
  ClassLiteral,
  "this",
  TypeName + '.' + "this",
  '(' + Expression + ')',
  ClassInstanceCreationExpression,
  FieldAccess,
  ArrayAccess,
  MethodInvocation,
  MethodReference)

rule ClassInstanceCreationExpression : ONEOF(
  UnqualifiedClassInstanceCreationExpression,
  ExpressionName + '.' + UnqualifiedClassInstanceCreationExpression,
  Primary + '.' + UnqualifiedClassInstanceCreationExpression)

rule UnqualifiedClassInstanceCreationExpression :
  "new" + ZEROORONE(TypeArguments) + ClassOrInterfaceTypeToInstantiate +
  '(' + ZEROORONE(ArgumentList) + ')' + ZEROORONE(ClassBody)
  attr.action : BuildNewOperation(%3, %5, %7)

rule ClassOrInterfaceTypeToInstantiate :
  ZEROORMORE(Annotation) + Identifier + ZEROORMORE('.' + ZEROORMORE(Annotation) + Identifier) +
  ZEROORONE(TypeArgumentsOrDiamond)

rule TypeArgumentsOrDiamond : ONEOF(
  TypeArguments,
  "<>")

rule ArgumentList : Expression + ZEROORMORE(',' + Expression)

rule ArrayCreationExpression : ONEOF(
  "new" + PrimitiveType + DimExprs + ZEROORONE(Dims),
  "new" + ClassOrInterfaceType + DimExprs + ZEROORONE(Dims),
  "new" + PrimitiveType + Dims + ArrayInitializer,
  "new" + ClassOrInterfaceType + Dims + ArrayInitializer)

rule DimExprs : DimExpr + ZEROORMORE(DimExpr)

rule DimExpr : ZEROORMORE(Annotation) + ZEROORONE(Expression)

rule ArrayAccess : ONEOF(
  ExpressionName + ZEROORONE(Expression),
  PrimaryNoNewArray + ZEROORONE(Expression))

rule FieldAccess : ONEOF(
  Primary + '.' + Identifier,
  "super" + '.' + Identifier,
  TypeName + '.' + "super" + '.' + Identifier)

rule MethodInvocation : ONEOF(
  MethodName + '(' + ZEROORONE(ArgumentList) + ')',
  TypeName + '.' + ZEROORONE(TypeArguments) + Identifier + '(' + ZEROORONE(ArgumentList) + ')',
  ExpressionName + '.' + ZEROORONE(TypeArguments) + Identifier + '(' + ZEROORONE(ArgumentList) + ')',
  Primary + '.' + ZEROORONE(TypeArguments) + Identifier + '(' + ZEROORONE(ArgumentList) + ')',
  "super" + '.' + ZEROORONE(TypeArguments) + Identifier + '(' + ZEROORONE(ArgumentList) + ')',
  TypeName + '.' + "super" + '.' + ZEROORONE(TypeArguments) + Identifier + '(' + ZEROORONE(ArgumentList) + ')')

rule ArgumentList : Expression + ZEROORMORE(',' + Expression)

rule MethodReference : ONEOF(
  ExpressionName + "::" + ZEROORONE(TypeArguments) + Identifier,
  Primary + "::" + ZEROORONE(TypeArguments) + Identifier,
  ReferenceType + "::" + ZEROORONE(TypeArguments) + Identifier,
  "super" + "::" + ZEROORONE(TypeArguments) + Identifier,
  TypeName + '.' + "super" + "::" + ZEROORONE(TypeArguments) + Identifier,
  ClassType + "::" + ZEROORONE(TypeArguments) "new",
  ArrayType + "::" + "new")

rule PostfixExpression : ONEOF(
  Primary,
  ExpressionName,
  PostIncrementExpression,
  PostDecrementExpression)

rule PostIncrementExpression : PostfixExpression + "++"
  attr.action : BuildPostfixOperation(%2, %1)
rule PostDecrementExpression : PostfixExpression + "--"
  attr.action : BuildPostfixOperation(%2, %1)

rule UnaryExpression : ONEOF(
  PreIncrementExpression,
  PreDecrementExpression,
  '+' + UnaryExpression,
  '-' + UnaryExpression,
  UnaryExpressionNotPlusMinus)
  attr.action.%3,%4 : BuildUnaryOperation(%1, %2)

rule PreIncrementExpression : "++" + UnaryExpression
  attr.action : BuildUnaryOperation(%1, %2)

rule PreDecrementExpression : "--" + UnaryExpression
  attr.action : BuildUnaryOperation(%1, %2)

rule UnaryExpressionNotPlusMinus : ONEOF(
  PostfixExpression,
  '~' + UnaryExpression,
  '!' + UnaryExpression,
  CastExpression)

rule CastExpression : ONEOF(
  '(' + PrimitiveType + ')' + UnaryExpression,
  '(' + ReferenceType + ZEROORMORE(AdditionalBound) + ')' + UnaryExpressionNotPlusMinus,
  '(' + ReferenceType + ZEROORMORE(AdditionalBound) + ')' + LambdaExpression)

rule CastExpression : ONEOF(
  '(' + PrimitiveType + ')' + UnaryExpression,
  '(' + ReferenceType + ZEROORMORE(AdditionalBound) + ')' + UnaryExpressionNotPlusMinus,
  '(' + ReferenceType + ZEROORMORE(AdditionalBound) + ')' + LambdaExpression)

rule MultiplicativeExpression : ONEOF(
  UnaryExpression,
  MultiplicativeExpression + '*' + UnaryExpression,
  MultiplicativeExpression + '/' + UnaryExpression,
  MultiplicativeExpression + '%' + UnaryExpression)

rule AdditiveExpression : ONEOF(
  MultiplicativeExpression,
  AdditiveExpression + '+' + MultiplicativeExpression,
  AdditiveExpression + '-' + MultiplicativeExpression)
  attr.action.%2,%3 : BuildBinaryOperation(%1, %2, %3)

rule ShiftExpression : ONEOF(
  AdditiveExpression,
  ShiftExpression + "<<" + AdditiveExpression,
  ShiftExpression + ">>" + AdditiveExpression,
  ShiftExpression + ">>>" + AdditiveExpression)
  attr.action.%2,%3,%4 : BuildBinaryOperation(%1, %2, %3)

rule RelationalExpression : ONEOF(
  ShiftExpression,
  RelationalExpression + '<' + ShiftExpression,
  RelationalExpression + '>' + ShiftExpression,
  RelationalExpression + "<=" + ShiftExpression,
  RelationalExpression + ">=" + ShiftExpression,
  RelationalExpression + "instanceof" + ReferenceType)
  attr.action.%2,%3,%4,%5 : BuildBinaryOperation(%1, %2, %3)

rule EqualityExpression : ONEOF(
  RelationalExpression,
  EqualityExpression + "==" + RelationalExpression,
  EqualityExpression + "!=" + RelationalExpression)

rule AndExpression : ONEOF(
  EqualityExpression,
  AndExpression + '&' + EqualityExpression)

rule ExclusiveOrExpression : ONEOF(
  AndExpression,
  ExclusiveOrExpression + '^' + AndExpression)

rule InclusiveOrExpression : ONEOF(
  ExclusiveOrExpression,
  InclusiveOrExpression + '|' + ExclusiveOrExpression)

rule ConditionalAndExpression : ONEOF(
  InclusiveOrExpression,
  ConditionalAndExpression + "&&" + InclusiveOrExpression)

rule ConditionalOrExpression : ONEOF(
  ConditionalAndExpression,
  ConditionalOrExpression + "||" + ConditionalAndExpression)

rule ConditionalExpression : ONEOF(
  ConditionalOrExpression,
  ConditionalOrExpression + '?' + Expression + ':' + ConditionalExpression,
  ConditionalOrExpression + '?' + Expression + ':' + LambdaExpression)

rule AssignmentExpression : ONEOF(
  ConditionalExpression,
  Assignment)

rule Assignment : LeftHandSide + AssignmentOperator + Expression
  attr.action : BuildAssignment(%1, %2, %3)

rule LeftHandSide : ONEOF(
  ExpressionName,
  FieldAccess,
  ArrayAccess)

rule AssignmentOperator : ONEOF('=', "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", ">>>=", "&=", "^=", "|=")

rule LambdaExpression : LambdaParameters + "->" + LambdaBody

rule LambdaParameters : ONEOF(
  '(' + ZEROORONE(LambdaParameterList) + ')',
  Identifier)

rule LambdaParameterList : ONEOF(
  LambdaParameter + ZEROORMORE(',' + LambdaParameter),
  Identifier + ZEROORMORE(',' + Identifier))

rule LambdaParameter : ONEOF(
  ZEROORMORE(VariableModifier) + LambdaParameterType + VariableDeclaratorId,
  VariableArityParameter)

rule LambdaParameterType : ONEOF(UnannType, "var")

rule VariableArityParameter : ZEROORMORE(VariableModifier) + UnannType + ZEROORMORE(Annotation) + "..." + Identifier

rule LambdaBody : ONEOF(Expression, Block)

rule ConstantExpression : Expression

rule Primary : ONEOF(
  PrimaryNoNewArray,
  ArrayCreationExpression)

rule Expression : ONEOF(
  ExpressionName,
  Primary,
  UnaryExpression,
  BinaryExpression,
  ConditionalExpression,
  LambdaExpression,
  AssignmentExpression)

rule BinaryExpression : ONEOF (
  MultiplicativeExpression,
  AdditiveExpression,
  ShiftExpression,
  RelationalExpression,
  EqualityExpression,
  AndExpression,
  ExclusiveOrExpression,
  InclusiveOrExpression,
  ConditionalAndExpression,
  ConditionalOrExpression)

