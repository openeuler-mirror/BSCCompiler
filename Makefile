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
# Makefile for OpenArkCompiler
OPT := O2
DEBUG := $(MAPLE_DEBUG)
OPS_ANDROID := 0
INSTALL_DIR := $(MAPLE_BUILD_OUTPUT)
MAPLE_BIN_DIR := $(MAPLE_ROOT)/src/mapleall/bin
MRT_ROOT := $(MAPLE_ROOT)/src/mrt
ANDROID_ROOT := $(MAPLE_ROOT)/android
ifeq ($(DEBUG),0)
  BUILD_TYPE := RELEASE
else
  BUILD_TYPE := DEBUG
endif
HOST_ARCH := 64
MIR_JAVA := 1
GN := $(MAPLE_ROOT)/tools/gn/gn
NINJA := $(shell ls $(MAPLE_ROOT)/tools/ninja*/ninja | tail -1)

GN_OPTIONS := \
  GN_INSTALL_PREFIX="$(MAPLE_ROOT)" \
  GN_BUILD_TYPE="$(BUILD_TYPE)" \
  HOST_ARCH=$(HOST_ARCH) \
  MIR_JAVA=$(MIR_JAVA) \
  OPT="$(OPT)" \
  OPS_ANDROID=$(OPS_ANDROID)

.PHONY: default
default: install

.PHONY: install_patch
install_patch:
	@bash build/third_party/patch.sh patch

.PHONY: uninstall_patch
uninstall_patch:
	@bash build/third_party/patch.sh unpatch

.PHONY: maplegen
maplegen:install_patch
	$(call build_gn, $(GN_OPTIONS), maplegen)

.PHONY: maplegendef
maplegendef: maplegen
	$(call build_gn, $(GN_OPTIONS), maplegendef)

.PHONY: maple
maple: maplegendef
	$(call build_gn, $(GN_OPTIONS), maple)

.PHONY: irbuild
irbuild:
	$(call build_gn, $(GN_OPTIONS), irbuild)

.PHONY: ast2mpl
ast2mpl:
	$(call build_gn, $(GN_OPTIONS), ast2mpl)

.PHONY: mplfe
mplfe: install_patch
	$(call build_gn, $(GN_OPTIONS), mplfe)

.PHONY: mplfeUT
mplfeUT:
	$(call build_gn, $(GN_OPTIONS) COV_CHECK=1, mplfeUT)

.PHONY: maple-rt
maple-rt: java-core-def
	$(call build_gn, $(GN_OPTIONS), maple-rt)

.PHONY: libcore
libcore: install maple-rt
	cd $(MAPLE_BUILD_OUTPUT)/libjava-core; \
	$(MAKE) install OPT=$(OPT) DEBUG=$(DEBUG) OPS_ANDROID=$(OPS_ANDROID)

.PHONY: java-core-def
java-core-def: install
	cp -rp $(MAPLE_ROOT)/libjava-core $(MAPLE_BUILD_OUTPUT)/; \
	cd $(MAPLE_BUILD_OUTPUT)/libjava-core; \
	ln -f -s $(MAPLE_ROOT)/build/core/libcore.mk ./makefile; \
	$(MAKE) gen-def OPT=$(OPT) DEBUG=$(DEBUG) OPS_ANDROID=$(OPS_ANDROID)

.PHONY: install
install: maple dex2mpl_install irbuild
	$(shell mkdir -p $(INSTALL_DIR)/ops/linker/; \
	rsync -a -L $(MRT_ROOT)/maplert/linker/maplelld.so.lds $(INSTALL_DIR)/ops/linker/; \
	rsync -a -L $(MAPLE_ROOT)/build/java2d8 $(INSTALL_DIR)/bin; \
	rsync -a -L $(MAPLE_BIN_DIR)/java2jar $(INSTALL_DIR)/bin/; \
	rsync -a -L $(MAPLE_BIN_DIR)/jbc2mpl $(INSTALL_DIR)/bin/;)

.PHONY: all
all: install mplfe libcore

ifeq ($(OPS_ANDROID),0)
.PHONY: dex2mpl_install
dex2mpl_install:
	$(shell rsync -a -L $(MAPLE_BIN_DIR)/dex2mpl $(INSTALL_DIR)/bin/;)
else
.PHONY: dex2mpl_install
dex2mpl_install:
	$(shell rsync -a -L $(MAPLE_BIN_DIR)/dex2mpl_android $(INSTALL_DIR)/bin/dex2mpl;)
endif

.PHONY: setup
setup:
	(cd tools; ./setup_tools.sh)

.PHONY: demo
demo:
	test/maple_aarch64_with_whirl2mpl.sh test/c_demo printHuawei 1

.PHONY: test1
test1: libcore
	python3 test/main.py test/testsuite/ouroboros/string_test/RT0001-rt-string-ReflectString/ReflectString.java --test_cfg=test/testsuite/ouroboros/test.cfg --verbose --debug

.PHONY: test_irbuild
test_irbuild: install
	python3 test/main.py test/testsuite/irbuild_test --test_cfg=test/testsuite/irbuild_test/test.cfg -j20 -pFAIL

.PHONY: test_ourboros
test_ourboros: libcore
	python3 test/main.py test/testsuite/ouroboros --test_cfg=test/testsuite/ouroboros/test.cfg --timeout=180 -j20 --retry 1 --fail_exit -pFAIL

.PHONY: c_test
c_test:
	$(MAKE) -C test c_test

.PHONY: testall
testall: 
	$(MAKE) -C test all

.PHONY: cleanrsd
cleanrsd:uninstall_patch
	@rm -rf libjava-core/libcore-all.* libjava-core/m* libjava-core/comb.*

.PHONY: clean
clean: cleanrsd
	@rm -rf $(MAPLE_BUILD_OUTPUT)/
	@rm -rf $(MAPLE_BUILD_OUTPUT)

.PHONY: clobber
clobber: cleanrsd
	@rm -rf output

define build_gn
    mkdir -p $(INSTALL_DIR); \
    $(GN) gen $(INSTALL_DIR) --args='$(1)' --export-compile-commands; \
    cd $(INSTALL_DIR); \
    $(NINJA) -v $(2);
endef
