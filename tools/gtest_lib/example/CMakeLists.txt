#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

cmake_minimum_required(VERSION 3.16)
project(gtest_example)

#set(CMAKE_CXX_STANDARD 17)
find_package(Threads REQUIRED)

add_executable(gtest_example test_example.cpp)
target_link_libraries(gtest_example PUBLIC ${CMAKE_SOURCE_DIR}/../lib/libgtest.a
                                           ${CMAKE_SOURCE_DIR}/../lib/libgtest_main.a
                                           Threads::Threads)
target_include_directories(gtest_example PUBLIC ../include)
