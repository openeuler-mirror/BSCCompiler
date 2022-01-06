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

STRUCT Operator : ONEOF(
                    # Arithmetic
                    ("+",    Add),
                    ("-",    Sub),
                    ("*",    Mul),
                    ("/",    Div),
                    ("%",    Mod),
                    ("++",   Inc),
                    ("--",   Dec),
                    # Relation
                    ("==",   EQ),
                    ("!=",   NE),
                    (">",    GT),
                    ("<",    LT),
                    (">=",   GE),
                    ("<=",   LE),
                    # Bitwise
                    ("&",    Band),
                    ("|",    Bor),
                    ("^",    Bxor),
                    ("~",    Bcomp),
                    # Shift
                    ("<<",   Shl),
                    (">>",   Shr),
                    # Logical
                    ("&&",   Land),
                    ("||",   Lor),
                    ("!",    Not),
                    # Assign
                    ("=",    Assign),
                    ("+=",   AddAssign),
                    ("-=",   SubAssign),
                    ("*=",   MulAssign),
                    ("/=",   DivAssign),
                    ("%=",   ModAssign),
                    ("<<=",  ShlAssign),
                    (">>=",  ShrAssign),
                    ("&=",   BandAssign),
                    ("|=",   BorAssign),
                    ("^=",   BxorAssign))