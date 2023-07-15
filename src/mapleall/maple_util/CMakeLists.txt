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

set(src_libmplutil
    src/mpl_logging.cpp
    src/muid.cpp
    src/namemangler.cpp
    src/literalstrname.cpp
    src/file_layout.cpp
    src/file_utils.cpp
    src/mpl_scheduler.cpp
    src/itab_util.cpp
    src/mpl_timer.cpp
    src/profile.cpp
    src/string_utils.cpp
    src/error_code.cpp
    src/thread_env.cpp
    src/mpl_int_val.cpp
    src/chain_layout.cpp
    src/mpl_profdata.cpp
    src/suffix_array.cpp
    src/mpl_posix_sighandler.cpp
    src/bit_value.cpp
    src/orig_symbol.cpp
)

set(src_libcommandline
    src/cl_option.cpp
    src/cl_option_parser.cpp
    src/cl_parser.cpp
)

# libmplutil
add_library(libmplutil STATIC ${src_libmplutil})
target_include_directories(libmplutil PRIVATE
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_be/include/cg
  ${MAPLEALL_ROOT}/maple_be/include/be
  ${MAPLEALL_ROOT}/mempool/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
  include
)
#set(THREADS_PREFER_PTHREAD_FLAG ON) # -pthread
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#target_link_libraries(libmplutil pthread)
set_target_properties(libmplutil PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
    #    STATIC_LIBRARY_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -lpthread -ldl"
)


#libcommandline
add_library(libcommandline STATIC ${src_libcommandline})
target_include_directories(libcommandline PRIVATE
    ${MAPLEALL_ROOT}/maple_util/include
    ${MAPLEALL_ROOT}/maple_ir/include
    ${MAPLEALL_ROOT}/maple_driver/include
    include
)
set_target_properties(libcommandline PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)






