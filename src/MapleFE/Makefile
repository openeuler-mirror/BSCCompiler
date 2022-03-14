# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reverved.
#
# OpenArkFE is licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#  http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

include Makefile.in

TARGS = autogen shared recdetect ladetect astopt java2mpl ast2mpl ts2ast ast2cpp c2ast obfuscate

# create BUILDDIR first
$(shell $(MKDIR_P) $(BUILDDIR))

ifeq ($(SRCLANG),java)
  TARGET := java2ast ast2mpl
else ifeq ($(SRCLANG),typescript)
  TARGET := ts2ast ast2cpp obfuscate
else ifeq ($(SRCLANG),c)
  TARGET := c2ast
endif

all: $(TARGET)

java2ast: autogen recdetect ladetect shared
	$(MAKE) LANG=$(SRCLANG) -C $(SRCLANG)

ts2ast: autogen recdetect ladetect shared
	$(MAKE) LANG=$(SRCLANG) -C $(SRCLANG)

c2ast: autogen recdetect ladetect shared astopt
	$(MAKE) LANG=$(SRCLANG) -C $(SRCLANG)

recdetect: autogen shared ladetect
	$(MAKE) LANG=$(SRCLANG) -C recdetect

ladetect: autogen shared
	$(MAKE) LANG=$(SRCLANG) -C ladetect

ast2mpl: astopt java2ast
	$(MAKE) -C ast2mpl

astopt: shared recdetect ladetect
	$(MAKE) -C astopt

ast2cpp: astopt ts2ast
	$(MAKE) -C ast2cpp

obfuscate: astopt ts2ast ast2cpp
	$(MAKE) -C tools/obfuscate

shared: autogen
	$(MAKE) -C shared

autogen:
	$(MAKE) -C autogen

mapleall:
	./scripts/build_mapleall.sh

test:
	$(MAKE) LANG=$(SRCLANG) -C test

testms:
	$(MAKE) LANG=$(SRCLANG) -C test testms

testall: test

test1:
	@cp test/java2mpl/t1.java .
	@echo gdb --args ./output/java/java2mpl t1.java --trace-a2m
	./output/java/java2mpl t1.java --trace-a2m
	cat t1.mpl

clean:
	rm -rf $(BUILDDIR)

clobber:
	rm -rf output

r: rebuild

rebuild:
	make clobber
	make -j

.PHONY: $(TARGS) test

