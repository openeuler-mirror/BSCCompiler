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
include $(MAPLE_BUILD_CORE)/maple_variables.mk

test: APP_RUN
include $(MAPLE_BUILD_CORE)/qemu_run.mk
include $(MAPLE_BUILD_CORE)/link.mk	
include $(MAPLE_BUILD_CORE)/mplcomb_dex.mk
include $(MAPLE_BUILD_CORE)/genmplt.mk
include $(MAPLE_BUILD_CORE)/hir2mpl_test.mk
include $(MAPLE_BUILD_CORE)/java2dex.mk

.PHONY: clean
clean:
	@rm -rf *.jar
	@rm -f *.class
	@rm -f *.mpl
	@rm -f *.mplt
	@rm -f *.s
	@rm -f *.groots.txt
	@rm -f *.primordials.txt
	@rm -rf comb.log
	@rm -rf *.muid
	@rm -rf *.dex
	@rm -rf *.def
	@rm -rf *.so
	@rm -rf *.o
