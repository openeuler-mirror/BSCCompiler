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

STRUCT Keyword : (("char", Char),
                  ("short", Short),
                  ("int", Int),
                  ("long", Long),
                  ("float", Float),
                  ("double", Double),
                  ("void", Void),
                  ("_Bool", Boolean))

rule BooleanType : "_Bool"
rule IntType : ONEOF("char", "unsigned" + "char", "signed" + "char",
                     "short", "unsigned" + "short", "signed" + "short",
                     "int", "unsigned" + "int", "signed" + "int", "unsigned", "signed",
                     "long", "unsigned" + "long", "signed" + "long")
rule FPType  : ONEOF("float", "double")
rule NumericType : ONEOF(IntType, FPType)

rule PrimitiveType : ONEOF(NumericType, BooleanType)
rule TypeVariable  : Identifier

rule NonePointerType : ONEOF(PrimitiveType, TypeVariable)
rule PointerType : NonePointerType + '*' + ZEROORMORE('*')
rule Type : ONEOF(NonePointerType, PointerType)