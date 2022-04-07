#
# Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
INSTALL_DIR := $(MAPLE_BUILD_OUTPUT)
LIB_CORE_PATH := $(MAPLE_BUILD_OUTPUT)/libjava-core/host-x86_64-$(OPT)
MAPLE_BIN_DIR := $(MAPLE_ROOT)/src/mapleall/bin
MRT_ROOT := $(MAPLE_ROOT)/src/mrt
ANDROID_ROOT := $(MAPLE_ROOT)/android
MAJOR_VERSION := $(MAPLE_MAJOR_VERSION)
MINOR_VERSION := $(MAPLE_MINOR_VERSION)
RELEASE_VERSION := $(MAPLE_RELEASE_VERSION)
BUILD_VERSION := $(MAPLE_BUILD_VERSION)
GIT_REVISION := $(shell  git log --pretty=format:"%H" -1)
MAST := 0
ASAN := 0
ifeq ($(DEBUG),0)
  BUILD_TYPE := RELEASE
else
  BUILD_TYPE := DEBUG
endif
HOST_ARCH := 64
MIR_JAVA := 1
GN := $(MAPLE_ROOT)/tools/gn/gn
NINJA := $(shell ls $(MAPLE_ROOT)/tools/ninja*/ninja | tail -1)
ifneq ($(findstring GC,$(OPT)),)
  GCONLY := 1
else
  GCONLY := 0
endif

GN_OPTIONS := \
  GN_INSTALL_PREFIX="$(MAPLE_ROOT)" \
  GN_BUILD_TYPE="$(BUILD_TYPE)" \
  HOST_ARCH=$(HOST_ARCH) \
  MIR_JAVA=$(MIR_JAVA) \
  OPT="$(OPT)" \
  GCONLY=$(GCONLY) \
  TARGET="$(TARGET_PROCESSOR)" \
  MAJOR_VERSION="$(MAJOR_VERSION)" \
  MINOR_VERSION="$(MINOR_VERSION)" \
  RELEASE_VERSION="$(RELEASE_VERSION)" \
  BUILD_VERSION="$(BUILD_VERSION)" \
  GIT_REVISION="$(GIT_REVISION)" \
  MAST=$(MAST) \
  ASAN=$(ASAN)

.PHONY: default
default: install

.PHONY: directory
directory:
	$(shell mkdir -p $(INSTALL_DIR)/bin;)

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
irbuild: install_patch
	$(call build_gn, $(GN_OPTIONS), irbuild)

.PHONY: mpldbg
mpldbg:
	$(call build_gn, $(GN_OPTIONS), mpldbg)

.PHONY: ast2mpl
ast2mpl:
	$(call build_gn, $(GN_OPTIONS), ast2mpl)

.PHONY: hir2mpl
hir2mpl: install_patch
	$(call build_gn, $(GN_OPTIONS), hir2mpl)

.PHONY: clang2mpl
clang2mpl: maple
	(cd tools/clang2mpl; make setup; make; make install)

.PHONY: hir2mplUT
hir2mplUT:
	$(call build_gn, $(GN_OPTIONS) COV_CHECK=1, hir2mplUT)

.PHONY: libcore
libcore: maple-rt
	cd $(LIB_CORE_PATH); \
	$(MAKE) install OPT=$(OPT) DEBUG=$(DEBUG)

.PHONY: maple-rt
maple-rt: java-core-def
	$(call build_gn, $(GN_OPTIONS), maple-rt)

.PHONY: mapleallUT
mapleallUT: install_patch
	$(call build_gn, $(GN_OPTIONS), mapleallUT)

.PHONY: java-core-def
java-core-def: install
	mkdir -p $(LIB_CORE_PATH); \
	cp -rp $(MAPLE_ROOT)/libjava-core/* $(LIB_CORE_PATH)/; \
	cd $(LIB_CORE_PATH); \
	ln -f -s $(MAPLE_ROOT)/build/core/libcore.mk ./makefile; \
	$(MAKE) gen-def OPT=$(OPT) DEBUG=$(DEBUG)

.PHONY: install
install: maple dex2mpl_install irbuild hir2mpl
	$(shell mkdir -p $(INSTALL_DIR)/ops/linker/; \
	rsync -a -L $(MRT_ROOT)/maplert/linker/maplelld.so.lds $(INSTALL_DIR)/ops/linker/; \
	rsync -a -L $(MAPLE_ROOT)/build/java2d8 $(INSTALL_DIR)/bin; \
	rsync -a -L $(MAPLE_BIN_DIR)/java2jar $(INSTALL_DIR)/bin/; \
	cp -rf $(MAPLE_ROOT)/tools $(INSTALL_DIR)/../; \
	rsync -a -L $(MAPLE_ROOT)/src/hir2mpl/ast_input/clang/lib/sys/ $(INSTALL_DIR)/lib/include/;)

.PHONY: all
all: install libcore

.PHONY: dex2mpl_install
dex2mpl_install: directory
	$(shell rsync -a -L $(MAPLE_BIN_DIR)/dex2mpl $(INSTALL_DIR)/bin/;)

.PHONY: setup
setup:
	(cd tools; ./setup_tools.sh)

.PHONY: demo
demo:
	test/maple_aarch64_with_hir2mpl.sh test/c_demo printHuawei 1 1
	test/maple_aarch64_with_clang2mpl.sh test/c_demo printHuawei 1 1

.PHONY: ctorture-ci
ctorture-ci:
	(cd third_party/ctorture; git checkout .; git pull; ./ci.sh)

.PHONY: ctorture
ctorture:
	(cd third_party/ctorture; git checkout .; git pull; ./run.sh work.list)

.PHONY: ctorture2
ctorture2:
	(cd third_party/ctorture; git checkout .; git pull; ./run.sh work.list hir2mpl)

THREADS := 50
ifneq ($(findstring test,$(MAKECMDGOALS)),)
TESTTARGET := $(MAKECMDGOALS)
ifdef TARGET
REALTARGET := $(TARGET)
else
REALTARGET := $(TESTTARGET)
endif
.PHONY: $(TESTTARGET)
${TESTTARGET}:
	@python3 $(MAPLE_ROOT)/testsuite/driver/src/driver.py --target=$(REALTARGET) --run-path=$(MAPLE_ROOT)/output/$(MAPLE_BUILD_TYPE)/testsuite $(if $(MOD), --mod=$(MOD),) --j=$(THREADS)
endif

.PHONY: cleanrsd
cleanrsd:uninstall_patch
	@rm -rf libjava-core/libcore-all.* libjava-core/m* libjava-core/comb.*

.PHONY: clean
clean: cleanrsd
	@rm -rf $(MAPLE_BUILD_OUTPUT)/
	@rm -rf $(MAPLE_ROOT)/report.txt

.PHONY: clobber
clobber: cleanrsd
	@rm -rf output

define build_gn
    mkdir -p $(INSTALL_DIR); \
    $(GN) gen $(INSTALL_DIR) --args='$(1)' --export-compile-commands; \
    cd $(INSTALL_DIR); \
    $(NINJA) -v $(2);
endef
