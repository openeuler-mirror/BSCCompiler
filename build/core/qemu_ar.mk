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
LIB_AR_PATH=$(MAPLE_OUT)/ar/host-x86_64-$(OPT)
LIB_RUNTIME=$(LIB_AR_PATH)/libmplcompiler-rt.a
LIB_ZTERP=$(LIB_AR_PATH)/libzterp.a
LIB_MRT=$(LIB_AR_PATH)/libmaplert.a
LIB_HUAWEISECUREC=$(LIB_AR_PATH)/libhuawei_secure_c.a
LIB_CORENATIVE=$(LIB_AR_PATH)/libcore-static-binding-jni.a
LIBDEXINTERFACE=$(LIB_AR_PATH)/libdexinterface.a
LIBCORE_SO_QEMU := $(MAPLE_OUT)/lib/$(OPT)/libcore-all.so
ifneq ($(findstring GC,$(OPT)),)
  LIB_CORENATIVE_QEMU=$(MAPLE_ROOT)/src/mrt/deplibs/libcore-static-binding-jni-qemu.a
else
  LIB_CORENATIVE_QEMU=$(MAPLE_ROOT)/src/mrt/deplibs/gc/libcore-static-binding-jni-qemu.a
endif

qemu := $(LIB_RUNTIME) $(LIB_MRT) $(LIB_HUAWEISECUREC) $(LIB_CORENATIVE_QEMU) $(LIB_CORENATIVE) $(LIB_ZTERP) $(LIBDEXINTERFACE)
