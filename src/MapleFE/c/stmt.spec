#
# Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

# based "on" C11 specification A.2 Phrase structure grammar

rule PrimaryExpression : ONEOF(
  Identifier,
  Literal,
  StringLiteral,
  '(' + Expression + ')',
  GenericSelection
)

rule GenericSelection : ONEOF(
  "_Generic" + '(' + AssignmentExpression + ',' + GenericAssocList + ')'
)

rule GenericAssocList : ONEOF(
  GenericAssociation,
  GenericAssocList + ',' + GenericAssociation
)

rule GenericAssociation : ONEOF(
  TypeName + ':' + AssignmentExpression,
  "default" + ':' + AssignmentExpression
)

rule PostfixExpression : ONEOF(
  PrimaryExpression,
  PostfixExpression + '[' + Expression + ']',
  PostfixExpression + '(' + ZEROORONE(ArgumentExpressionList) + ')',
  PostfixExpression + '.' + Identifier,
  PostfixExpression + "->" + Identifier,
  PostfixExpression + "++",
  PostfixExpression + "--",
  '(' + TypeName + ')' + '{' + InitializerList + '}',
  '(' + TypeName + ')' + '{' + InitializerList + ',' + '}'
)

rule ArgumentExpressionList : ONEOF(
  AssignmentExpression,
  ArgumentExpressionList + ',' + AssignmentExpression
)

rule UnaryExpression : ONEOF(
  PostfixExpression,
  "++" + UnaryExpression,
  "--" + UnaryExpression,
  UnaryOperator + CastExpression,
  "sizeof" + UnaryExpression,
  "sizeof" + '(' + TypeName + ')',
  "_Alignof" + '(' + TypeName + ')'
)

rule UnaryOperator: ONEOF(
  '&', '*', '+', '-', '~', '!'
)

rule CastExpression : ONEOF(
  UnaryExpression,
  '(' + TypeName + ')' + CastExpression
)

rule MultiplicativeExpression : ONEOF(
  CastExpression,
  MultiplicativeExpression + '*' + CastExpression,
  MultiplicativeExpression + '/' + CastExpression,
  MultiplicativeExpression + '%' + CastExpression
)

rule AdditiveExpression : ONEOF(
  MultiplicativeExpression,
  AdditiveExpression + '+' + MultiplicativeExpression,
  AdditiveExpression + '-' + MultiplicativeExpression
)

rule ShiftExpression : ONEOF(
  AdditiveExpression,
  ShiftExpression + "<<" + AdditiveExpression,
  ShiftExpression + ">>" + AdditiveExpression
)

rule RelationalExpression : ONEOF(
  ShiftExpression,
  RelationalExpression + '<' + ShiftExpression,
  RelationalExpression + '>' + ShiftExpression,
  RelationalExpression + "<=" + ShiftExpression,
  RelationalExpression + ">=" + ShiftExpression
)

rule EqualityExpression : ONEOF(
  RelationalExpression,
  EqualityExpression + "==" + RelationalExpression,
  EqualityExpression + "!=" + RelationalExpression
)

rule ANDExpression : ONEOF(
  EqualityExpression,
  ANDExpression + '&' + EqualityExpression
)

rule ExclusiveORExpression : ONEOF(
  ANDExpression,
  ExclusiveORExpression + '^' + ANDExpression
)

rule InclusiveORExpression : ONEOF(
  ExclusiveORExpression,
  InclusiveORExpression + '|' + ExclusiveORExpression
)

rule LogicalANDExpression : ONEOF(
  InclusiveORExpression,
  LogicalANDExpression + "&&" + InclusiveORExpression
)

rule LogicalORExpression : ONEOF(
  LogicalANDExpression,
  LogicalORExpression + "||" + LogicalANDExpression
)

rule ConditionalExpression : ONEOF(
  LogicalORExpression,
  LogicalORExpression + '?' + Expression + ':' + ConditionalExpression
)

rule AssignmentExpression : ONEOF(
  ConditionalExpression,
  UnaryExpression + AssignmentOperator + AssignmentExpression
)

rule AssignmentOperator: ONEOF(
  '=', "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|="
)
  attr.property : Single

rule Expression : ONEOF(
  AssignmentExpression,
  Expression + ',' + AssignmentExpression
)

rule ConstantExpression : ONEOF(
  ConditionalExpression
)

rule Declaration : ONEOF(
  DeclarationSpecifiers + ZEROORONE(InitDeclaratorList) + ';',
  Static_assertDeclaration
)
  attr.property : Single

rule DeclarationSpecifiers : ONEOF(
  StorageClassSpecifier + ZEROORONE(DeclarationSpecifiers),
  TypeSpecifier + ZEROORONE(DeclarationSpecifiers),
  TypeQualifier + ZEROORONE(DeclarationSpecifiers),
  FunctionSpecifier + ZEROORONE(DeclarationSpecifiers),
  AlignmentSpecifier + ZEROORONE(DeclarationSpecifiers)
)

rule InitDeclaratorList : ONEOF(
  InitDeclarator,
  InitDeclaratorList + ',' + InitDeclarator
)

rule InitDeclarator : ONEOF(
  Declarator,
  Declarator + '=' + Initializer
)

rule StorageClassSpecifier : ONEOF(
  "typedef",
  "extern",
  "static",
  "_Thread_local",
  "auto",
  "register"
)
  attr.property : Single

rule TypeSpecifier : ONEOF(
  "void",
  "char",
  "short",
  "int",
  "long",
  "float",
  "double",
  "signed",
  "unsigned",
  "_Bool",
  "_Complex",
  AtomicTypeSpecifier,
  StructOrUnionSpecifier,
  EnumSpecifier,
  TypedefName
)
  attr.property : Single

rule StructOrUnionSpecifier : ONEOF(
  StructOrUnion + ZEROORONE(Identifier) + '{' + StructDeclarationList + '}',
  StructOrUnion + Identifier
)

rule StructOrUnion : ONEOF(
  "struct",
  "union"
)
  attr.property : Single

rule StructDeclarationList : ONEOF(
  StructDeclaration,
  StructDeclarationList + StructDeclaration
)

rule StructDeclaration : ONEOF(
  SpecifierQualifierList + ZEROORONE(StructDeclaratorList) + ';',
  Static_assertDeclaration
)

rule SpecifierQualifierList : ONEOF(
  TypeSpecifier + ZEROORONE(SpecifierQualifierList),
  TypeQualifier + ZEROORONE(SpecifierQualifierList)
)

rule StructDeclaratorList : ONEOF(
  StructDeclarator,
  StructDeclaratorList + ',' + StructDeclarator
)

rule StructDeclarator : ONEOF(
  Declarator,
  ZEROORONE(Declarator) + ':' + ConstantExpression
)

rule EnumSpecifier : ONEOF(
  "enum" + ZEROORONE(Identifier) + '{' + EnumeratorList + '}',
  "enum" + ZEROORONE(Identifier) + '{' + EnumeratorList + ',' + '}',
  "enum" + Identifier
)

rule EnumeratorList : ONEOF(
  Enumerator,
  EnumeratorList + ',' + Enumerator
)

rule Enumerator : ONEOF(
  EnumerationConstant,
  EnumerationConstant + '=' + ConstantExpression
)

rule AtomicTypeSpecifier : ONEOF(
  "_Atomic" + '(' + TypeName + ')'
)

rule TypeQualifier : ONEOF(
  "const",
  "restrict",
  "volatile",
  "_Atomic"
)

rule FunctionSpecifier : ONEOF(
  "inline",
  "_Noreturn"
)

rule AlignmentSpecifier : ONEOF(
  "_Alignas" + '(' + TypeName + ')',
  "_Alignas" + '(' + ConstantExpression + ')'
)

rule Declarator : ONEOF(
  ZEROORONE(Pointer) + DirectDeclarator
)

rule DirectDeclarator : ONEOF(
  Identifier,
  '(' + Declarator + ')',
  DirectDeclarator + '[' + ZEROORONE(TypeQualifierList) + ZEROORONE(AssignmentExpression) + ']',
  DirectDeclarator + '[' + "static" + ZEROORONE(TypeQualifierList) + AssignmentExpression + ']',
  DirectDeclarator + '[' + TypeQualifierList + "static" + AssignmentExpression + ']',
  DirectDeclarator + '[' + ZEROORONE(TypeQualifierList) + '*' + ']',
  DirectDeclarator + '(' + ParameterTypeList + ')',
  DirectDeclarator + '(' + ZEROORONE(IdentifierList) + ')'
)

rule Pointer : ONEOF(
  '*' + ZEROORONE(TypeQualifierList),
  '*' + ZEROORONE(TypeQualifierList) + Pointer
)

rule TypeQualifierList : ONEOF(
  TypeQualifier,
  TypeQualifierList + TypeQualifier
)

rule ParameterTypeList : ONEOF(
  ParameterList,
  ParameterList + ',' + "..."
)

rule ParameterList : ONEOF(
  ParameterDeclaration,
  ParameterList + ',' + ParameterDeclaration
)

rule ParameterDeclaration : ONEOF(
  DeclarationSpecifiers + Declarator,
  DeclarationSpecifiers + ZEROORONE(AbstractDeclarator)
)

rule IdentifierList : ONEOF(
  Identifier,
  IdentifierList + ',' + Identifier
)

rule TypeName : ONEOF(
  SpecifierQualifierList + ZEROORONE(AbstractDeclarator)
)

rule AbstractDeclarator : ONEOF(
  Pointer,
  ZEROORONE(Pointer) + DirectAbstractDeclarator
)

rule DirectAbstractDeclarator : ONEOF(
  '(' + AbstractDeclarator + ')',
  ZEROORONE(DirectAbstractDeclarator) + '[' + ZEROORONE(TypeQualifierList),
  ZEROORONE(AssignmentExpression) + ']',
  ZEROORONE(DirectAbstractDeclarator) + '[' + "static" + ZEROORONE(TypeQualifierList),
  AssignmentExpression + ']',
  ZEROORONE(DirectAbstractDeclarator) + '[' + TypeQualifierList + "static",
  AssignmentExpression + ']',
  ZEROORONE(DirectAbstractDeclarator) + '[' + '*' + ']',
  ZEROORONE(DirectAbstractDeclarator) + '(' + ZEROORONE(ParameterTypeList) + ')'
)

rule TypedefName : ONEOF(
  Identifier
)

rule Initializer : ONEOF(
  AssignmentExpression,
  '{' + InitializerList + '}',
  '{' + InitializerList + ',' + '}'
)

rule InitializerList : ONEOF(
  ZEROORONE(Designation) + Initializer,
  InitializerList + ',' + ZEROORONE(Designation) + Initializer
)

rule Designation : ONEOF(
  DesignatorList + '='
)

rule DesignatorList : ONEOF(
  Designator,
  DesignatorList + Designator
)

rule Designator : ONEOF(
  '[' + ConstantExpression + ']',
  '.' + Identifier
)

rule Static_assertDeclaration : ONEOF(
  "_Static_assert" + '(' + ConstantExpression + ',' + StringLiteral + ')' + ';'
)

rule Statement : ONEOF(
  LabeledStatement,
  CompoundStatement,
  ExpressionStatement,
  SelectionStatement,
  IterationStatement,
  JumpStatement
)
  attr.property : Top

rule LabeledStatement : ONEOF(
  Identifier + ':' + Statement,
  "case" + ConstantExpression + ':' + Statement,
  "default" + ':' + Statement
)
  attr.property : Single

rule CompoundStatement : ONEOF(
  '{' + ZEROORONE(BlockItemList) + '}'
)

rule BlockItemList : ONEOF(
  BlockItem,
  BlockItemList + BlockItem
)

rule BlockItem : ONEOF(
  Declaration,
  Statement
)

rule ExpressionStatement : ONEOF(
  ZEROORONE(Expression) + ';'
)

rule SelectionStatement : ONEOF(
  "if" + '(' + Expression + ')' + Statement,
  "if" + '(' + Expression + ')' + Statement + "else" + Statement,
  "switch" + '(' + Expression + ')' + Statement
)
  attr.property : Single

rule IterationStatement : ONEOF(
  "while" + '(' + Expression + ')' + Statement,
  "do" + Statement + "while" + '(' + Expression + ')' + ';',
  "for" + '(' + ZEROORONE(Expression) + ';' + ZEROORONE(Expression) + ';' + ZEROORONE(Expression) + ')' + Statement,
  "for" + '(' + Declaration + ZEROORONE(Expression) + ';' + ZEROORONE(Expression) + ')' + Statement
)
  attr.property : Single

rule JumpStatement : ONEOF(
  "goto" + Identifier + ';',
  "continue" + ';',
  "break" + ';',
  "return" + ZEROORONE(Expression) + ';'
)
  attr.property : Single

rule TranslationUnit : ONEOF(
  ExternalDeclaration,
  TranslationUnit + ExternalDeclaration
)

rule ExternalDeclaration : ONEOF(
  Declaration,
  FunctionDefinition
)
  attr.property : Top
  attr.property : Single

rule FunctionDefinition : ONEOF(
  DeclarationSpecifiers + Declarator + ZEROORONE(DeclarationList) + CompoundStatement
)

rule DeclarationList : ONEOF(
  Declaration,
  DeclarationList + Declaration
)

rule preprocessingFile: ONEOF(
  ZEROORONE(Group)
)

rule Group: ONEOF(
  GroupPart,
  Group + GroupPart
)

rule GroupPart: ONEOF(
  IfSection,
  ControlLine,
  TextLine,
  '#' + NonDirective
)

rule IfSection: ONEOF(
  IfGroup + ZEROORONE(ElifGroups) + ZEROORONE(ElseGroup) + EndifLine
)

rule IfGroup: ONEOF(
  '#' + "if" + ConstantExpression + NewLine + ZEROORONE(Group),
  '#' + "ifdef" + Identifier + NewLine + ZEROORONE(Group),
  '#' + "ifndef" + Identifier + NewLine + ZEROORONE(Group)
)
  attr.property : Single

rule ElifGroups: ONEOF(
  ElifGroup,
  ElifGroups + ElifGroup
)

rule ElifGroup: ONEOF(
  '#' + "elif" + ConstantExpression + NewLine + ZEROORONE(Group)
)

rule ElseGroup: ONEOF(
  '#' + "else" + NewLine + ZEROORONE(Group)
)

rule EndifLine: ONEOF(
  '#' + "endif" + NewLine
)

rule ControlLine: ONEOF(
  '#' + "include" + PpTokens + NewLine,
  '#' + "define" + Identifier + ReplacementList + NewLine,
  '#' + "define" + Identifier + Lparen + ZEROORONE(IdentifierList) + ')' + ReplacementList + NewLine,
  '#' + "define" + Identifier + Lparen + "..." + ')' + ReplacementList + NewLine,
  '#' + "define" + Identifier + Lparen + IdentifierList + ',' + "..." + ')' + ReplacementList + NewLine,
  '#' + "undef" + Identifier + NewLine,
  '#' + "line" + PpTokens + NewLine,
  '#' + "error" + ZEROORONE(PpTokens) + NewLine,
  '#' + "pragma" + ZEROORONE(PpTokens) + NewLine,
  '#' + NewLine
)
  attr.property : Single

rule TextLine: ONEOF(
  ZEROORONE(PpTokens) + NewLine
)

rule NonDirective: ONEOF(
  PpTokens + NewLine
)

rule Lparen: ONEOF(
  '('
)

rule ReplacementList: ONEOF(
  ZEROORONE(PpTokens)
)

rule PpTokens: ONEOF(
  PreprocessingToken,
  PpTokens + PreprocessingToken
)

rule NewLine: '\' + 'n'

rule PreprocessingToken: ONEOF(
  HeaderName,
  Identifier,
  PpNumber,
  CharacterLiteral,
  StringLiteral,
  Punctuator,
  #Each nonWhiteSpace character that cannot be one of the above
)

rule Punctuator: ONEOF(
  '[', ']', '(', ')', '{', '}', '.', "->",
  "++", "--", '&', '*', '+', '-', '~', '!', '/', '%',
  "<<", ">>", '<', '>', "<=", ">=", "==", "!=", '^', '|', "&&", "||",
  '?', ':', ';', "...",
  '=', "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=",
  ',', '#', "##", "<:", ":>", "<%", "%>", "%:", "%:%:"
)

rule HeaderName: ONEOF(
  '<' + HCharSequence + '>',
  '"' + QCharSequence + '"'
)

rule HCharSequence: ONEOF(
  HChar,
  HCharSequence + HChar
)

rule HChar: ONEOF(CHAR, DIGIT, '-', '_')

rule QCharSequence: ONEOF(
  QChar,
  QCharSequence + QChar
)

rule QChar: ONEOF(CHAR, DIGIT, '-', '_')

rule PpNumber: ONEOF(
  DIGIT,
  '.' + DIGIT,
  PpNumber + DIGIT,
  PpNumber + ONEOF(CHAR, '_'),
  PpNumber + 'e' + Sign0,
  PpNumber + 'E' + Sign0,
  PpNumber + 'p' + Sign0,
  PpNumber + 'P' + Sign0,
  PpNumber + '.'
)

rule Sign0 : ONEOF('+', '-')

