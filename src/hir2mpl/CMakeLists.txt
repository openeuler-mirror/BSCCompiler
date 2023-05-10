#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN AS IS BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#
if (${COV_CHECK} STREQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_COV_CHECK=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_COV_CHECK=1")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMIR_FEATURE_FULL=1 -DHIR2MPL_FULL_INFO_DUMP=1 -DJAVA_OBJ_IN_MFILE=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMIR_FEATURE_FULL=1 -DHIR2MPL_FULL_INFO_DUMP=1 -DJAVA_OBJ_IN_MFILE=1")
set(CMAKE_SKIP_RPATH TRUE)

if (${ONLY_C} STREQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -DONLY_C")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -DONLY_C")
endif()

if (${MAST} STREQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -DENABLE_MAST")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -DENABLE_MAST")
endif()

set(include_directories
  ${HIR2MPL_ROOT}/common/include
  ${HIR2MPL_ROOT}/optimize/include
  ${HIR2MPL_ROOT}/bytecode_input/class/include
  ${HIR2MPL_ROOT}/bytecode_input/common/include
  ${HIR2MPL_ROOT}/bytecode_input/dex/include
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/mempool/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
  ${HIR2MPL_ROOT}/ast_input/clang/include
  ${HIR2MPL_ROOT}/ast_input/clang/lib
  ${HIR2MPL_ROOT}/ast_input/common/include
  ${HIR2MPL_ROOT}/ast_input/maple/include
  ${HIR2MPL_ROOT}/ast_input/maple/lib
  ${LLVMINC}
  ${THIRD_PARTY_ROOT}/llvm_modified/clang/tools
)

if (${MAST} STREQUAL "1")
  set(include_directories
    ${include_directories}
    ${MAPLE_PARSER_PATH}/output/c/ast_gen/shared
    ${MAPLE_PARSER_PATH}/shared/include
    ${MAPLE_PARSER_PATH}/astopt/include
  )
endif()

set(src_lib_hir2mpl_common
  ${HIR2MPL_ROOT}/common/src/base64.cpp
  ${HIR2MPL_ROOT}/common/src/basic_io.cpp
  ${HIR2MPL_ROOT}/common/src/fe_config_parallel.cpp
  ${HIR2MPL_ROOT}/common/src/fe_file_ops.cpp
  ${HIR2MPL_ROOT}/common/src/fe_file_type.cpp
  ${HIR2MPL_ROOT}/common/src/fe_function.cpp
  ${HIR2MPL_ROOT}/common/src/fe_function_phase_result.cpp
  ${HIR2MPL_ROOT}/common/src/fe_input_helper.cpp
  ${HIR2MPL_ROOT}/common/src/fe_java_string_manager.cpp
  ${HIR2MPL_ROOT}/common/src/fe_manager.cpp
  ${HIR2MPL_ROOT}/common/src/fe_options.cpp
  ${HIR2MPL_ROOT}/common/src/fe_struct_elem_info.cpp
  ${HIR2MPL_ROOT}/common/src/fe_timer_ns.cpp
  ${HIR2MPL_ROOT}/common/src/fe_type_hierarchy.cpp
  ${HIR2MPL_ROOT}/common/src/fe_type_manager.cpp
  ${HIR2MPL_ROOT}/common/src/fe_utils.cpp
  ${HIR2MPL_ROOT}/common/src/fe_utils_ast.cpp
  ${HIR2MPL_ROOT}/common/src/fe_utils_java.cpp
  ${HIR2MPL_ROOT}/common/src/feir_builder.cpp
  ${HIR2MPL_ROOT}/common/src/feir_stmt.cpp
  ${HIR2MPL_ROOT}/common/src/feir_type.cpp
  ${HIR2MPL_ROOT}/common/src/feir_type_helper.cpp
  ${HIR2MPL_ROOT}/common/src/feir_type_infer.cpp
  ${HIR2MPL_ROOT}/common/src/feir_var.cpp
  ${HIR2MPL_ROOT}/common/src/feir_var_name.cpp
  ${HIR2MPL_ROOT}/common/src/feir_var_reg.cpp
  ${HIR2MPL_ROOT}/common/src/feir_var_type_scatter.cpp
  ${HIR2MPL_ROOT}/common/src/hir2mpl_compiler.cpp
  ${HIR2MPL_ROOT}/common/src/hir2mpl_compiler_component.cpp
  ${HIR2MPL_ROOT}/common/src/hir2mpl_env.cpp
  ${HIR2MPL_ROOT}/common/src/hir2mpl_options.cpp
  ${HIR2MPL_ROOT}/common/src/hir2mpl_option.cpp
  ${HIR2MPL_ROOT}/common/src/simple_xml.cpp
  ${HIR2MPL_ROOT}/common/src/simple_zip.cpp
  ${HIR2MPL_ROOT}/common/src/generic_attrs.cpp
  ${HIR2MPL_ROOT}/common/src/enhance_c_checker.cpp
  ${HIR2MPL_ROOT}/common/src/feir_scope.cpp
)
if (NOT ${MAJOR_VERSION} STREQUAL "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAJOR_VERSION=${MAJOR_VERSION}")
endif()
if (NOT ${MINOR_VERSION} STREQUAL "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMINOR_VERSION=${MINOR_VERSION}")
endif()
if (NOT ${RELEASE_VERSION} STREQUAL "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRELEASE_VERSION=\\\"${RELEASE_VERSION}\\\"")
endif()
if (NOT ${BUILD_VERSION} STREQUAL "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_VERSION=${BUILD_VERSION}")
endif()
if (NOT ${GIT_REVISION} STREQUAL "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_REVISION=\\\"${GIT_REVISION}\\\"")
endif()
#lib_hir2mpl_common
add_library(lib_hir2mpl_common STATIC ${src_lib_hir2mpl_common})
set_target_properties(lib_hir2mpl_common PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_directories}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/ar"
    COMPILE_FLAGS ""
)

set(src_lib_hir2mpl_optimize
  ${HIR2MPL_ROOT}/optimize/src/ror.cpp
  ${HIR2MPL_ROOT}/optimize/src/conditional_operator.cpp
  ${HIR2MPL_ROOT}/optimize/src/feir_lower.cpp
  ${HIR2MPL_ROOT}/optimize/src/feir_bb.cpp
  ${HIR2MPL_ROOT}/optimize/src/feir_cfg.cpp
  ${HIR2MPL_ROOT}/optimize/src/feir_dfg.cpp
)

#lib_hir2mpl_optimize
add_library(lib_hir2mpl_optimize STATIC ${src_lib_hir2mpl_optimize})
set_target_properties(lib_hir2mpl_optimize PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_directories}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/ar"
)

#lib_hir2mpl_input_helper
add_library(lib_hir2mpl_input_helper STATIC "${HIR2MPL_ROOT}/common/src/fe_input_helper.cpp")
set_target_properties(lib_hir2mpl_input_helper PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_directories}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/ar"
)

set(src_hir2mpl
  ${HIR2MPL_ROOT}/common/src/hir2mpl.cpp
)

set(deps_hir2mpl
    ${LLVMLIBDIR}/libclang.so
    ${LLVMLIBDIR}/libclang-cpp.so
    ${LLVMLIBDIR}/libclangFrontend.a
    ${LLVMLIBDIR}/libclangDriver.a
    ${LLVMLIBDIR}/libclangSerialization.a
    ${LLVMLIBDIR}/libclangParse.a
    ${LLVMLIBDIR}/libclangSema.a
    ${LLVMLIBDIR}/libclangEdit.a
    ${LLVMLIBDIR}/libclangLex.a
    ${LLVMLIBDIR}/libclangAnalysis.a
    ${LLVMLIBDIR}/libclangAST.a
    ${LLVMLIBDIR}/libclangBasic.a
    ${LLVMLIBDIR}/libLLVMDemangle.a
    ${LLVMLIBDIR}/libLLVMMCParser.a
    ${LLVMLIBDIR}/libLLVMMC.a
    ${LLVMLIBDIR}/libLLVMBitReader.a
    ${LLVMLIBDIR}/libLLVMCore.a
    ${LLVMLIBDIR}/libLLVMBinaryFormat.a
    ${LLVMLIBDIR}/libLLVMProfileData.a
    ${LLVMLIBDIR}/libLLVMOption.a
    ${LLVMLIBDIR}/libLLVMSupport.a
    lib_hir2mpl_ast_input_clang_lib
    libmplphase
    libcommandline
    lib_hir2mpl_ast_input_clang
    lib_hir2mpl_ast_input_common
    lib_hir2mpl_input_helper
    lib_hir2mpl_common
    lib_hir2mpl_optimize
    libdriver_option
    libmaple_driver
    libmplir
    libmplutil
    libmempool
    libmpl2mpl
    libHWSecureC
)

if (NOT ${ONLY_C} STREQUAL "1")
    list(APPEND deps_hir2mpl
		lib_hir2mpl_bytecode_input_common
		lib_hir2mpl_bytecode_input_dex
		lib_hir2mpl_bytecode_input_class
		libdexfile
		libziparchive
		libbase
		${THIRD_PARTY_ROOT}/aosp_modified/system/core/liblog/liblog.a
    )
endif()
if (${MAST} STREQUAL "1")
    list(APPEND deps_hir2mpl
		lib_hir2mpl_ast_input_maple_lib
	    lib_hir2mpl_ast_input_maple
    )
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lz -rdynamic -L${LLVMLIBDIR}/")

if (${MAST} STREQUAL "1")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++fs -L${MAPLE_PARSER_PATH}/output/c/ast_gen/shared")
endif()

if (${COV} STREQUAL "1")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if (${GPROF} STREQUAL "1")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

#hir2mpl
add_executable(hir2mpl "${src_hir2mpl}")
set_target_properties(hir2mpl PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_directories}"
    LINK_LIBRARIES "${deps_hir2mpl}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"

)

set(inc_lib_hir2mpl_bytecode_input_class
  ${HIR2MPL_ROOT}/common/include
  ${HIR2MPL_ROOT}/optimize/include
  ${HIR2MPL_ROOT}/bytecode_input/class/include
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/mempool/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)

set(src_lib_hir2mpl_bytecode_input_class
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_attr.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_attr_item.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_bb.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_class.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_class2fe_helper.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_class_const.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_class_const_pool.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_compiler_component.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_function.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_function_context.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_input.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_opcode.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_opcode_helper.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_stack2fe_helper.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_stack_helper.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_stmt.cpp
  ${HIR2MPL_ROOT}/bytecode_input/class/src/jbc_util.cpp
)

# lib_hir2mpl_bytecode_input_class
add_library(lib_hir2mpl_bytecode_input_class STATIC ${src_lib_hir2mpl_bytecode_input_class})
set_target_properties(lib_hir2mpl_bytecode_input_class PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_lib_hir2mpl_bytecode_input_class}"
  LINK_LIBRARIES ""
  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
)

if (NOT ${ONLY_C} STREQUAL "1")
	set(inc_lib_hir2mpl_bytecode_input_common
      ${HIR2MPL_ROOT}/common/include
      ${HIR2MPL_ROOT}/optimize/include
      ${HIR2MPL_ROOT}/bytecode_input/common/include
      ${HIR2MPL_ROOT}/bytecode_input/dex/include
      ${MAPLEALL_ROOT}/maple_ir/include
      ${MAPLEALL_ROOT}/maple_util/include
      ${MAPLEALL_ROOT}/maple_pgo/include
      ${MAPLEALL_ROOT}/maple_driver/include
      ${MAPLEALL_ROOT}/mempool/include
      ${THIRD_PARTY_ROOT}/bounds_checking_function/include
	)

	set(src_lib_hir2mpl_bytecode_input_common
      ${HIR2MPL_ROOT}/bytecode_input/common/src/ark_annotation_map.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/ark_annotation_processor.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_class.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_class2fe_helper.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_function.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_instruction.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_io.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_parser_base.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_pragma.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/bc_util.cpp
      ${HIR2MPL_ROOT}/bytecode_input/common/src/rc_setter.cpp
	)
	# lib_hir2mpl_bytecode_input_common
	add_library(lib_hir2mpl_bytecode_input_common STATIC ${src_lib_hir2mpl_bytecode_input_common})
	set_target_properties(lib_hir2mpl_bytecode_input_common PROPERTIES
	  COMPILE_FLAGS ""
	  INCLUDE_DIRECTORIES "${inc_lib_hir2mpl_bytecode_input_common}"
	  LINK_LIBRARIES ""
	  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
	)

	set(include_bytecode_input_dex_directories
      ${HIR2MPL_ROOT}/common/include
      ${HIR2MPL_ROOT}/optimize/include
      ${HIR2MPL_ROOT}/bytecode_input/common/include
      ${HIR2MPL_ROOT}/bytecode_input/dex/include
      ${MAPLEALL_ROOT}/maple_ir/include
      ${MAPLEALL_ROOT}/maple_util/include
      ${MAPLEALL_ROOT}/maple_pgo/include
      ${MAPLEALL_ROOT}/maple_driver/include
      ${MAPLEALL_ROOT}/mempool/include
      ${THIRD_PARTY_ROOT}/bounds_checking_function/include
	)

	set(src_lib_hir2mpl_bytecode_input_dex
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/class_linker.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/class_loader_context.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_class.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_class2fe_helper.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_encode_value.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_file_util.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_op.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_pragma.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_strfac.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_parser.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dex_reader.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dexfile_factory.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dexfile_interface.cpp
      ${HIR2MPL_ROOT}/bytecode_input/dex/src/dexfile_libdexfile.cpp
	)

	list(APPEND include_bytecode_input_dex_directories
	  ${THIRD_PARTY_ROOT}/aosp_modified/system/core/include
	)

	list(APPEND include_bytecode_input_dex_directories
      ${THIRD_PARTY_ROOT}/aosp_modified/system/core/liblog/include
      ${THIRD_PARTY_ROOT}/aosp_modified/system/core/libutils/include
      ${THIRD_PARTY_ROOT}/aosp_modified/system/core/base/include
      ${THIRD_PARTY_ROOT}/aosp_modified/system/core/libziparchive/include
      ${THIRD_PARTY_ROOT}/aosp_modified/art/libartpalette/include
      ${THIRD_PARTY_ROOT}/aosp_modified/art/libartbase
      ${THIRD_PARTY_ROOT}/aosp_modified/art/libdexfile
      ${THIRD_PARTY_ROOT}/aosp_modified/include
      ${THIRD_PARTY_ROOT}/aosp_modified/libnativehelper/include_jni
	)

	set(deps_libdexfile
	  libdexfile
	  libziparchive
	  libbase
	  ${THIRD_PARTY_ROOT}/aosp_modified/system/core/liblog/liblog.a
	)

	# lib_hir2mpl_bytecode_input_dex
	add_library(lib_hir2mpl_bytecode_input_dex STATIC ${src_lib_hir2mpl_bytecode_input_dex})
	set_target_properties(lib_hir2mpl_bytecode_input_dex PROPERTIES
	  COMPILE_FLAGS ""
	  INCLUDE_DIRECTORIES "${include_bytecode_input_dex_directories}"
	  LINK_LIBRARIES "${deps_libdexfile}"
	  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
	)
endif()

set(src_lib_hir2mpl_ast_input_clang_lib
  ${HIR2MPL_ROOT}/ast_input/clang/lib/ast_interface.cpp
)

set(deps_lib_hir2mpl_ast_input_clang_lib
  ${LLVMLIBDIR}/libclang.so
  ${LLVMLIBDIR}/libclang-cpp.so
  ${LLVMLIBDIR}/libclangFrontend.a
  ${LLVMLIBDIR}/libclangDriver.a
  ${LLVMLIBDIR}/libclangSerialization.a
  ${LLVMLIBDIR}/libclangParse.a
  ${LLVMLIBDIR}/libclangSema.a
  ${LLVMLIBDIR}/libclangEdit.a
  ${LLVMLIBDIR}/libclangLex.a
  ${LLVMLIBDIR}/libclangAnalysis.a
  ${LLVMLIBDIR}/libclangAST.a
  ${LLVMLIBDIR}/libclangBasic.a
  ${LLVMLIBDIR}/libLLVMDemangle.a
  ${LLVMLIBDIR}/libLLVMMCParser.a
  ${LLVMLIBDIR}/libLLVMMC.a
  ${LLVMLIBDIR}/libLLVMBitReader.a
  ${LLVMLIBDIR}/libLLVMCore.a
  ${LLVMLIBDIR}/libLLVMBinaryFormat.a
  ${LLVMLIBDIR}/libLLVMProfileData.a
  ${LLVMLIBDIR}/libLLVMOption.a
  ${LLVMLIBDIR}/libLLVMSupport.a
)

set(defines
  CLANG_ENABLE_ARCMT
  CLANG_ENABLE_OBJC_REWRITER
  CLANG_ENABLE_STATIC_ANALYZER
  GTEST_HAS_RTTI=0
  _DEBUG
  _GNU_SOURCE
  __STDC_CONSTANT_MACROS
  __STDC_FORMAT_MACROS
  __STDC_LIMIT_MACROS
)

# lib_hir2mpl_ast_input_clang_lib
add_library(lib_hir2mpl_ast_input_clang_lib STATIC ${src_lib_hir2mpl_ast_input_clang_lib})
set_target_properties(lib_hir2mpl_ast_input_clang_lib PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${include_directories}"
  LINK_LIBRARIES "${deps_lib_hir2mpl_ast_input_clang_lib}"
  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
  DEFINE_SYMBOL "${defines}"
)

# lib_hir2mpl_ast_input_common
add_library(lib_hir2mpl_ast_input_common STATIC "${HIR2MPL_ROOT}/ast_input/common/src/ast_decl.cpp")
set_target_properties(lib_hir2mpl_ast_input_common PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${include_directories}"
  LINK_LIBRARIES ""
  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
)

set(src_lib_hir2mpl_ast_input_clang
  ${HIR2MPL_ROOT}/ast_input/clang/lib/ast_type.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/lib/ast_util.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/src/ast_expr.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/src/ast_function.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/src/ast_parser.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/src/ast_parser_builting_func.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/src/ast_stmt.cpp
  ${HIR2MPL_ROOT}/ast_input/clang/src/ast_struct2fe_helper.cpp
)

set(deps_lib_hir2mpl_ast_input_clang
  lib_hir2mpl_ast_input_clang_lib
)

# lib_hir2mpl_ast_input_clang
add_library(lib_hir2mpl_ast_input_clang STATIC ${src_lib_hir2mpl_ast_input_clang})
set_target_properties(lib_hir2mpl_ast_input_clang PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${include_directories}"
  LINK_LIBRARIES "${deps_lib_hir2mpl_ast_input_clang}"
  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
)

if (${MAST} STREQUAL "1")
	set(deps_lib_hir2mpl_ast_input_maple_lib
      ${MAPLE_PARSER_PATH}/output/c/ast_gen/shared/genast.a
      ${MAPLE_PARSER_PATH}/output/c/shared/shared.a
      ${MAPLE_PARSER_PATH}/output/c/gen/gen.a
      ${MAPLE_PARSER_PATH}/output/c/astopt/astopt.a
	)
# lib_hir2mpl_ast_input_maple_lib
	add_library(lib_hir2mpl_ast_input_maple_lib STATIC "${HIR2MPL_ROOT}/ast_input/maple/lib/maple_ast_interface.cpp")
	set_target_properties(lib_hir2mpl_ast_input_maple_lib PROPERTIES
	  COMPILE_FLAGS ""
	  INCLUDE_DIRECTORIES "${include_directories}"
	  LINK_LIBRARIES "${deps_lib_hir2mpl_ast_input_maple_lib}"
	  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
	  DEFINE_SYMBOL "DEBUG"
	)
# lib_hir2mpl_ast_input_maple
	add_library(lib_hir2mpl_ast_input_maple STATIC "${HIR2MPL_ROOT}/ast_input/maple/src/maple_ast_parser.cpp")
	set_target_properties(lib_hir2mpl_ast_input_maple PROPERTIES
	  COMPILE_FLAGS ""
	  INCLUDE_DIRECTORIES "${include_directories}"
	  LINK_LIBRARIES "lib_hir2mpl_ast_input_maple_lib"
	  RUNTIME_OUTPUT_DIRECTORY "${root_out_dir}/ar"
	  DEFINE_SYMBOL "DEBUG"
	)
endif()
