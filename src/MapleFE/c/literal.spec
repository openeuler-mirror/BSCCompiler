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
#
# A literal is the source code representation of a value of a primitive type, the
# String type, or the null type.
#
# NOTE : Make sure there is a 'rule Literal'. This is the official rule recognized
#       by autogen.

# based on C11 specification A.1 Lexical grammar

rule UniversalCharacterName : ONEOF(
  '\' + 'u' + HexQuad,
  '\' + 'U' + HexQuad + HexQuad
)

rule HexQuad : ONEOF(
  HexadecimalDigit + HexadecimalDigit + HexadecimalDigit + HexadecimalDigit
)

rule IntegerLiteral : ONEOF(
  DecimalConstant + ZEROORONE(IntegerSuffix),
#  OctalConstant + ZEROORONE(IntegerSuffix),
#  HexadecimalConstant + ZEROORONE(IntegerSuffix)
)

rule DecimalConstant : ONEOF(
  '0', NonzeroDigit + ZEROORMORE(DIGIT)
)

rule OctalConstant : ONEOF(
  '0',
  OctalConstant + OctalDigit
)

rule HexadecimalConstant : ONEOF(
  HexadecimalPrefix + HexadecimalDigit,
  HexadecimalConstant + HexadecimalDigit
)

rule HexadecimalPrefix : ONEOF(
  '0' + 'x', '0' + 'X'
)

rule NonzeroDigit : ONEOF(
  '1', '2', '3', '4', '5', '6', '7', '8', '9'
)

rule OctalDigit : ONEOF(
  '0', '1', '2', '3', '4', '5', '6', '7'
)

rule HexadecimalDigit : ONEOF(
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'A', 'B', 'C', 'D', 'E', 'F'
)

rule IntegerSuffix : ONEOF(
  UnsignedSuffix + ZEROORONE(LongSuffix),
  UnsignedSuffix + LongLongSuffix,
  LongSuffix + ZEROORONE(UnsignedSuffix),
  LongLongSuffix + ZEROORONE(UnsignedSuffix)
)

rule UnsignedSuffix : ONEOF(
  'u', 'U'
)

rule LongSuffix : ONEOF(
  'l', 'L'
)

rule LongLongSuffix : ONEOF(
  'l' + 'l', 'L' + 'L'
)

rule FPLiteral : ONEOF(
  DecimalFloatingConstant,
  HexadecimalFloatingConstant
)

rule DecimalFloatingConstant : ONEOF(
  FractionalConstant + ZEROORONE(ExponentPart) + ZEROORONE(FloatingSuffix),
  DigitSequence + ExponentPart + ZEROORONE(FloatingSuffix)
)

rule HexadecimalFloatingConstant : ONEOF(
  HexadecimalPrefix + HexadecimalFractionalConstant,
  BinaryExponentPart + ZEROORONE(FloatingSuffix),
  HexadecimalPrefix + HexadecimalDigitSequence,
  BinaryExponentPart + ZEROORONE(FloatingSuffix)
)

rule FractionalConstant : ONEOF(
  ZEROORONE(DigitSequence) + '.' + DigitSequence,
  DigitSequence '.'
)

rule ExponentPart : ONEOF(
  'e' + ZEROORONE(Sign) + DigitSequence,
  'E' + ZEROORONE(Sign) + DigitSequence
)

rule Sign : ONEOF(
  '+', '-'
)

rule DigitSequence : ONEOF(
  DIGIT,
  DIGIT + ZEROORMORE(DIGIT) + DIGIT
)
  attr.property.%2 : SecondTry

rule HexadecimalFractionalConstant : ONEOF(
  ZEROORONE(HexadecimalDigitSequence) + '.',
  HexadecimalDigitSequence,
  HexadecimalDigitSequence + '.'
)

rule BinaryExponentPart : ONEOF(
  'p' + ZEROORONE(Sign) + DigitSequence,
  'P' + ZEROORONE(Sign) + DigitSequence
)

rule HexadecimalDigitSequence : ONEOF(
  HexadecimalDigit,
  HexadecimalDigitSequence + HexadecimalDigit
)

rule FloatingSuffix : ONEOF(
  'f', 'l', 'F', 'L'
)

rule EnumerationConstant : Identifier

rule CharacterConstant : ONEOF(
  ''' + SCharSequence + ''',
  'L' + ''' + SCharSequence + ''',
  'U' + ''' + SCharSequence + ''',
  'U' + ''' + SCharSequence + '''
)

rule CCharSequence : ONEOF(
  CChar,
  CCharSequence + CChar
)

rule EscapeSequence : ONEOF(
  ESCAPE,
  OctalEscapeSequence,
  HexadecimalEscapeSequence,
  UniversalCharacterName
)

rule OctalEscapeSequence : ONEOF(
  '\' + OctalDigit,
  '\' + OctalDigit + OctalDigit,
  '\' + OctalDigit + OctalDigit + OctalDigit
)

rule HexadecimalEscapeSequence : ONEOF(
  '\' + 'x' + HexadecimalDigit,
  HexadecimalEscapeSequence + HexadecimalDigit
)

rule StringLiteral : ONEOF(
  ZEROORONE(EncodingPrefix) + '"' + ZEROORONE(SCharSequence) + '"'
)

rule EncodingPrefix : ONEOF(
  'u' + '8', 'u', 'U', 'L'
)

rule SCharSequence : ONEOF(
  SChar,
  SCharSequence + SChar
)

rule SChar : ONEOF(
  CChar,
  EscapeSequence
)

#######################################
#rule CChar : ONEOF(
#   CHAR
#   '_'
#   EscapeSequence
#)

rule Digit : DIGIT

rule NullLiteral : "NULL"
rule BooleanLiteral : ONEOF("true", "false")
rule CharacterLiteral : CharacterConstant

rule Literal : ONEOF(
  IntegerLiteral,
  FPLiteral,
  EnumerationConstant,
  CharacterLiteral,
  StringLiteral,
  NullLiteral
)

