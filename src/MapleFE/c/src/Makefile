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

include ../../Makefile.in
BUILD=$(BUILDDIR)/c
$(shell $(MKDIR_P) $(BUILD) $(BUILDLIB) $(BUILDGEN))

SRC=$(wildcard *.cpp)
OBJ :=$(patsubst %.cpp,%.o,$(SRC))
DEP :=$(patsubst %.cpp,%.d,$(SRC))

SRCG := $(wildcard $(BUILDGEN)/gen*.cpp)
OBJG := $(patsubst %.cpp, %.o, $(SRCG))
DEPG := $(patsubst %.cpp, %.d, $(SRCG))

OBJS :=$(foreach obj,$(OBJ), $(BUILD)/$(obj)) $(OBJG)
DEPS :=$(foreach dep,$(DEP), $(BUILD)/$(dep)) $(DEPG)

INCLUDES := -I $(MAPLEFE_ROOT)/shared/include \
            -I $(MAPLEFE_ROOT)/c/include \
            -I $(MAPLEFE_ROOT)/autogen/include \
            -I ${BUILDDIR}/ast_gen/shared \
            -I $(BUILDGEN) \
            $(MAPLEALL_INC)

INCLUDEGEN := -I $(BUILDGEN) -I $(MAPLEFE_ROOT)/shared/include

TARGET=c2ast

SHAREDLIB = -L $(BUILDLIB) -lshared -lgenast

.PHONY: all
all: $(BUILDBIN)/$(TARGET)

-include $(DEPS)
.PHONY: clean

vpath %.o $(BUILD)
vpath %.d $(BUILD)

# Pattern Rules
$(BUILD)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -fpermissive $(INCLUDES) -w -c $< -o $@

$(BUILD)/%.d : %.cpp
	@$(CXX) $(CXXFLAGS) -std=c++11 -MM $(INCLUDES) $< > $@
	@mv -f $(BUILD)/$*.d $(BUILD)/$*.d.tmp
	@sed -e 's|.*:|$(BUILD)/$*.o:|' < $(BUILD)/$*.d.tmp > $(BUILD)/$*.d
	@rm -f $(BUILD)/$*.d.tmp

$(BUILDGEN)/%.o : $(BUILDGEN)/%.cpp $(BUILDGEN)/%.d
	$(CXX) $(CXXFLAGS) -fpermissive $(INCLUDEGEN) -w -c $< -o $@

$(BUILDGEN)/%.d : $(BUILDGEN)/%.cpp
	@$(CXX) $(CXXFLAGS) -std=c++11 -MM $(INCLUDEGEN) $< > $@
	@mv -f $(BUILDGEN)/$*.d $(BUILDGEN)/$*.d.tmp
	@sed -e 's|.*:|$(BUILDGEN)/$*.o:|' < $(BUILDGEN)/$*.d.tmp > $(BUILDGEN)/$*.d
	@rm -f $(BUILDGEN)/$*.d.tmp

# TARGET depends on OBJS and shared OBJS from shared directory
# as well as mapleall libraries
$(BUILDBIN)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILDBIN)
	$(LD) -o $(BUILDBIN)/$(TARGET) $(OBJS) $(SHAREDLIB) $(MAPLELIBS) -lstdc++fs

clean:
	rm -rf $(BUILD)
