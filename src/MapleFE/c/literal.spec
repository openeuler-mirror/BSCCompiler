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

#########################################################################
##                          Integer                                    ##
#########################################################################

### Decimal rules

rule NonZeroDigit   : ONEOF('1', '2', '3', '4', '5', '6', '7', '8', '9')
rule Digit          : ONEOF('0', NonZeroDigit)
rule DecimalNumeral : ONEOF('0', NonZeroDigit + ZEROORONE(Digit))

### Hexadecimal rules

rule HexDigit   : ONEOF('0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                        'a', 'b', 'c', 'd', 'e', 'f',
                        'A', 'B', 'C', 'D', 'E', 'F')
rule HexNumeral : ONEOF("0x" + HexDigit, "0X" + HexDigit)

### Octal rules

rule OctalDigit   : ONEOF('0', '1', '2', '3', '4', '5', '6', '7')
rule OctalNumeral : ONEOF('0' + OctalDigit)

rule IntegerTypeSuffix : ONEOF('L', 'l')
rule DecimalIntegerLiteral: DecimalNumeral + ZEROORONE(IntegerTypeSuffix)
rule HexIntegerLiteral    : HexNumeral + ZEROORONE(IntegerTypeSuffix)
rule OctalIntegerLiteral  : OctalNumeral + ZEROORONE(IntegerTypeSuffix)

rule IntegerLiteral: ONEOF(DecimalIntegerLiteral,
                           HexIntegerLiteral,
                           OctalIntegerLiteral)

#########################################################################
##                       Floating Point                                ##
#########################################################################

##### Decimal floating point literal

rule Sign : ONEOF('+', '-')
rule FloatTypeSuffix : ONEOF('f', 'F')
rule ExponentIndicator : ONEOF('e', 'E')
rule SignedInteger : ZEROORONE(Sign) + Digit
rule ExponentPart : ExponentIndicator + SignedInteger

rule DecFPLiteral : ONEOF(Digit + '.' + ZEROORONE(Digit) + ZEROORONE(ExponentPart) + ZEROORONE(FloatTypeSuffix),
                    '.'+Digit + ZEROORONE(ExponentPart) + ZEROORONE(FloatTypeSuffix),
                    Digit + ExponentPart + ZEROORONE(FloatTypeSuffix),
                    Digit + ZEROORONE(ExponentPart))

####### Hex floating point literal

rule BinaryExponentIndicator : ONEOF('p', 'P')
rule BinaryExponent : BinaryExponentIndicator + SignedInteger
rule HexSignificand : ONEOF(HexNumeral + ZEROORONE('.'),
                            "0x" + ZEROORONE(HexDigit) + '.' + HexDigit,
                            "0X" + ZEROORONE(HexDigit) + '.' + HexDigit)
rule HexFPLiteral: HexSignificand + BinaryExponent + ZEROORONE(FloatTypeSuffix)

######  Floating Point Literal

rule FPLiteral : ONEOF(DecFPLiteral, HexFPLiteral)

#########################################################################
##                           Boolean                                   ##
#########################################################################

rule BooleanLiteral : ONEOF ("true", "false")

#########################################################################
##                           Character                                 ##
## ESCAPE is a reserved rule in reserved.spec.                         ##
#########################################################################

rule UnicodeEscape: '\' + 'u' + HEXDIGIT + HEXDIGIT + HEXDIGIT + HEXDIGIT
rule RawInputCharacter : ONEOF(ASCII, ''', ESCAPE)
rule SingleCharacter: ONEOF(UnicodeEscape, RawInputCharacter)

rule OctalEscape : ONEOF('\' + '0', '\' + '1')
rule EscapeSequence : ONEOF(ESCAPE, OctalEscape)
rule CharacterLiteral : ''' + ONEOF(SingleCharacter, EscapeSequence) + '''

#########################################################################
##                           String                                    ##
#########################################################################
# The UnicodeEscape is limited from \u0000 to \u00ff.
rule StringUnicodeEscape: '\' + 'u' + '0' + '0' + HEXDIGIT + HEXDIGIT
rule StringCharater: ONEOF(StringUnicodeEscape, RawInputCharacter)
rule StringLiteral : '"' + ZEROORMORE(StringCharater) + '"'

#########################################################################
##                           Null                                      ##
#########################################################################

rule NullLiteral : "NULL"

#########################################################################
##                           Literal                                   ##
#########################################################################

rule Literal : ONEOF(IntegerLiteral,
                     FPLiteral,
                     BooleanLiteral,
                     CharacterLiteral,
                     StringLiteral,
                     NullLiteral)