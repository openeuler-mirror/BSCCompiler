set(inc_dirs
  ${MAPLEALL_ROOT}/maple_san/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/mempool/include
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_be/include/cg
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)

set(src_libmplsan
  src/asan_phases.cpp
  src/asan_mapping.cpp
  src/asan_stackvar.cpp
  src/asan_module.cpp
  src/asan_function.cpp
  src/san_common.cpp
  src/ubsan_phases.cpp
  src/ubsan_bounds.cpp
  src/san_phase_manager.cpp
)

add_library(libmplsan SHARED ${src_libmplsan})
set_target_properties(libmplsan PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_dirs}"
  LINK_LIBRARIES ""
  ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)

# OPTION(ASAN_ENABLE_RBTREE "Use RBTree for shadow memory management in ASan" OFF)
# if(ASAN_ENABLE_RBTREE)
#         target_compile_definitions(mplsan PUBLIC ENABLERBTREE)
# endif(ASAN_ENABLE_RBTREE)
