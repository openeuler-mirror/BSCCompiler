//
// Created by wchenbt on 2021/3/28.
//

#ifndef MAPLE_SAN_INCLUDE_ASAN_MESSAGES_H
#define MAPLE_SAN_INCLUDE_ASAN_MESSAGES_H

namespace maple {

const char *const kAsanModuleCtorName = "asan.module_ctor";
const char *const kAsanModuleDtorName = "asan.module_dtor";
const char *const kAsanInitName = "__asan_init";
const char *const kAsanHandleNoReturnName = "__asan_handle_no_return";
const char *const kAsanRegisterGlobalsName = "__asan_register_globals";
const char *const kAsanUnregisterGlobalsName = "__asan_unregister_globals";
const char *const kAsanSetShadowPrefix = "__asan_set_shadow_";
const char *const kAsanReportErrorTemplate = "__asan_report_";
const char *const kAsanShadowMemoryDynamicAddress = "__asan_shadow_memory_dynamic_address";
const char *const kAsanAllocaPoison = "__asan_alloca_poison";
const char *const kAsanAllocasUnpoison = "__asan_allocas_unpoison";

} // namespace maple
#endif // MAPLE_SAN_INCLUDE__ASAN_MESSAGES_H
