#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
LIB_RUNTIME=$(MAPLE_OUT)/ar/libmplcompiler-rt.a
LIB_ZTERP =$(MAPLE_OUT)/ar/libzterp.a
LIB_MRT=$(MAPLE_OUT)/ar/libmaplert.a
LIB_HUAWEISECUREC=$(MAPLE_OUT)/ar/libhuawei_secure_c.a
LIB_CORENATIVE=$(MAPLE_OUT)/ar/libcore-static-binding-jni.a
LIBDEXINTERFACE=$(MAPLE_OUT)/ar/libdexinterface.a
LIBCORE_SO_QEMU := $(MAPLE_OUT)/lib/$(OPT)/libcore-all.so
LIB_CORENATIVE_QEMU=$(MAPLE_ROOT)/src/mrt/deplibs/libcore-static-binding-jni-qemu.a

ifeq ($(OPS_ANDROID), 0)
  qemu := $(LIB_RUNTIME) $(LIB_MRT) $(LIB_HUAWEISECUREC) $(LIB_CORENATIVE_QEMU) $(LIB_CORENATIVE) $(LIB_ZTERP) $(LIBDEXINTERFACE)
else
  qemu := $(LIB_RUNTIME) $(LIB_MRT) $(LIB_CORENATIVE) $(LIB_ZTERP) $(LIBDEXINTERFACE)
endif
