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

# Version Info
if(NOT ${MAJOR_VERSION} STREQUAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAJOR_VERSION=${MAJOR_VERSION}")
endif()
if(NOT ${MINOR_VERSION} STREQUAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMINOR_VERSION=${MINOR_VERSION}")
endif()
if(NOT ${RELEASE_VERSION} STREQUAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRELEASE_VERSION=\\\"${RELEASE_VERSION}\\\"")
endif()
if(NOT ${BUILD_VERSION} STREQUAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_VERSION=${BUILD_VERSION}")
endif()
if(NOT ${GIT_REVISION} STREQUAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_REVISION=\\\"${GIT_REVISION}\\\"")
endif()

OPTION(ENABLE_MAPLE_SAN "Enabling sanitizer functionalities" OFF)
if(ENABLE_MAPLE_SAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_MAPLE_SAN=1")
endif(ENABLE_MAPLE_SAN)

add_subdirectory(maple_util)
add_subdirectory(mempool)
add_subdirectory(maple_ipa)
add_subdirectory(maple_phase)
add_subdirectory(mpl2mpl)
add_subdirectory(maple_me)
add_subdirectory(maple_be)
add_subdirectory(maple_driver)
add_subdirectory(maple_ir)
add_subdirectory(maple_pgo)
add_subdirectory(maple_san)