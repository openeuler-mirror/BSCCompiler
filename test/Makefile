#
# Copyright (c) [2021] Futurewei Technologies, Inc. All rights reverved.
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

.PHONY: demo
demo:
	make -C c_demo

.PHONY: all
all: test_irbuild test_ourboros c_test

.PHONY: test1
test1:
	python3 main.py testsuite/ouroboros/string_test/RT0001-rt-string-ReflectString/ReflectString.java --test_cfg=testsuite/ouroboros/test.cfg --verbose --debug

.PHONY: test_irbuild
test_irbuild:
	python3 main.py testsuite/irbuild_test --test_cfg=testsuite/irbuild_test/test.cfg -j20 -pFAIL

.PHONY: test_ourboros
test_ourboros:
	python3 main.py testsuite/ouroboros --test_cfg=testsuite/ouroboros/test.cfg --timeout=180 -j20 --retry 1 --fail_exit -pFAIL

.PHONY: c_test
c_test:
	make -C c_test

.PHONY: clean
clean:
	@rm -rf $(MAPLE_BUILD_OUTPUT)/test
