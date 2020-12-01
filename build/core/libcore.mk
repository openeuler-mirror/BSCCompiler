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
APP := libcore-all
include $(MAPLE_BUILD_CORE)/maple_variables.mk
ifeq ($(DEBUG), 1)
  MPLCG_FLAGS := $(MPLCG_FLAGS) --add-debug-trace
endif
include $(MAPLE_BUILD_CORE)/qemu_ar.mk

ifeq ($(DEBUG), 0)
  QEMU_CXX_FLAGS := -O2 -s
else
  QEMU_CXX_FLAGS := -O0 -g3
endif

ifeq ($(DISABLE_RC_DUPLICATE), 1)
  RC_FLAGS = -DDISABLE_RC_DUPLICATE=1
else
  RC_FLAGS = -DDISABLE_RC_DUPLICATE=0
endif

ifeq ($(OPS_ANDROID), 0)
  LINKER_QEMU_OPT := -fuse-ld=lld -rdynamic -Wl,-Bsymbolic -lpthread -ldl -L$(MAPLE_OUT)/lib/ -L$(MAPLE_ROOT)/third_party/icu/lib/aarch64-linux-gnu -licuuc
else
  LINKER_QEMU_OPT := -fuse-ld=lld -rdynamic -Wl,-Bsymbolic -ldl -L$(MAPLE_OUT)/lib/
	LINKER_QEMU_OPT += -L$(MAPLE_ROOT)/android/out/target/product/generic_arm64/apex/com.android.runtime/lib64 \
		-L$(MAPLE_ROOT)/android/out/target/product/generic_arm64/apex/com.android.runtime/lib64/bionic \
		-L$(MAPLE_ROOT)/android/out/target/product/generic_arm64/system/lib64 \
		-L$(MAPLE_ROOT)/src/mrt/deplibs \
		-Wl,-soname=libcore-all.so -lcommon_bridge -llog -lc_secshared -lbase -lc -lutils -lcutils -lc++ -lm -ldl
endif

$(APP_O): %.VtableImpl.o : %.VtableImpl.s
	$(QEMU_CLANG_CPP) $(QEMU_CXX_FLAGS) $(QEMU_CLANG_FLAGS) $(RC_FLAGS) -c -o $@ $<

ifeq ($(OPS_ANDROID), 0)
$(APP_QEMU_SO): %.VtableImpl.qemu.so : %.VtableImpl.o $(INIT_CXX_O) $(qemu)
	$(QEMU_CLANG_CPP) $(QEMU_CXX_FLAGS) $(QEMU_CLANG_FLAGS) $(RC_FLAGS) -shared -o $@ $(INIT_CXX_O) -Wl,--whole-archive $(qemu) -Wl,--no-whole-archive $(LINKER_QEMU_OPT) $< -Wl,-T $(LDS)
else
$(APP_QEMU_SO): %.VtableImpl.qemu.so : %.VtableImpl.o $(INIT_CXX_O) $(qemu)
	$(QEMU_CLANG_CPP) $(QEMU_CXX_FLAGS) $(QEMU_CLANG_FLAGS) $(RC_FLAGS) -shared -o $@ \
	$(MAPLE_ROOT)/android/out/soong/.intermediates/bionic/libc/crtbegin_so/android_arm64_armv8-a/crtbegin_so.o \
	$(INIT_CXX_O) -Wl,--whole-archive $(qemu) -Wl,--no-whole-archive $(LINKER_QEMU_OPT) $< \
	$(ANDROID_CLANG_PATH)/lib64/clang/9.0.3/lib/linux/libclang_rt.builtins-aarch64-android.a \
	$(ANDROID_GCC_PATH)/linux-x86/aarch64/aarch64-linux-android-4.9/aarch64-linux-android/lib64/libatomic.a \
	$(ANDROID_GCC_PATH)/linux-x86/aarch64/aarch64-linux-android-4.9/lib/gcc/aarch64-linux-android/4.9.x/libgcc.a \
	$(MAPLE_ROOT)/android/out/soong/.intermediates/bionic/libc/crtend_so/android_arm64_armv8-a/obj/bionic/libc/arch-common/bionic/crtend_so.o -Wl,-T $(LDS) \
	$(ANDROID_CLANG_PATH)/lib64/clang/9.0.3/lib/linux/libclang_rt.ubsan_minimal-aarch64-android.a \
	-Wl,--exclude-libs,libclang_rt.ubsan_minimal-aarch64-android.a -Wl,-execute-only
endif

include $(MAPLE_BUILD_CORE)/extra.mk
include $(MAPLE_BUILD_CORE)/mplcomb_dex.mk
include $(MAPLE_BUILD_CORE)/dex2mpl.mk

$(APP_DEX): %.dex : $(D8) $(LIB_CORE_JAR)
	$(D8) --min-api 39 --output . $(LIB_CORE_JAR)
	mv classes.dex $(APP_DEX)

.PHONY: gen-def
gen-def: $(APP_DEF)
$(APP_DEF): $(APP_S)
	rsync -a -L $(APP_DEF) $(MAPLE_ROOT)/src/mrt/unified.macros.def

.PHONY: install
install: libcore_so deplibs
	$(shell mkdir -p $(MAPLE_OUT)/ops/host-x86_64-$(OPT); \
	mkdir -p $(MAPLE_OUT)/ops/third_party; \
	rsync -a -L $(MAPLE_OUT)/lib/$(OPT)/libcore-all.so $(MAPLE_OUT)/ops/host-x86_64-$(OPT); \
	rsync -a -L $(MAPLE_ROOT)/libjava-core/mrt_module_init.o $(MAPLE_OUT)/ops/; \
	rsync -a -L $(MAPLE_ROOT)/libjava-core/libcore-all.mplt $(MAPLE_OUT)/ops/; \
	rsync -a -L $(MAPLE_ROOT)/third_party/libnativehelper $(MAPLE_OUT)/ops/; \
	rsync -a -L $(MAPLE_ROOT)/android/out/target/common/obj/JAVA_LIBRARIES $(MAPLE_OUT)/ops/third_party; \
	rsync -a -L $(MAPLE_ROOT)/third_party/libdex/prebuilts/aarch64-linux-gnu/libz.so.1.2.8 $(MAPLE_OUT)/ops/third_party/libz.so.1; \
	rsync -a -L $(MAPLE_ROOT)/third_party/icu/lib/aarch64-linux-gnu/* $(MAPLE_OUT)/ops/third_party/; \
	)

ifeq ($(OPS_ANDROID),0)
.PHONY: deplibs
deplibs:
	$(shell	rsync -a -L $(MAPLE_ROOT)/src/mrt/deplibs/*.so $(MAPLE_OUT)/ops/host-x86_64-$(OPT); \
	rsync -a -L $(MAPLE_ROOT)/src/mrt/bin/mplsh $(MAPLE_OUT)/ops/;)
else
.PHONY: deplibs
deplibs:
	$(shell	rsync -a -L $(MAPLE_ROOT)/src/mrt/deplibs/*.so $(MAPLE_OUT)/ops/host-x86_64-$(OPT); \
	rsync -a -L $(MAPLE_ROOT)/src/mrt/bin/mplsh_android $(MAPLE_OUT)/ops/mplsh;)
endif

.PHONY: libcore_so
libcore_so: $(LIBCORE_SO_QEMU)
$(LIBCORE_SO_QEMU): $(APP_QEMU_SO)
	mkdir -p $(MAPLE_OUT)/lib/$(OPT)/
	rsync -a -L $< $@
clean:
	@rm -f libcore-all.*
	@rm -f *.mpl
	@rm -f *.mplt
	@rm -f *.groots.txt
	@rm -f *.primordials.txt
	@rm -rf comb.log
	@rm -rf *.muid
	@rm -f *.s
	@rm -rf *.o
