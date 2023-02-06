//
// Created by wchenbt on 2021/3/28.
//

#ifndef MAPLE_SAN_INCLUDE_ASAN_MESSAGES_H
#define MAPLE_SAN_INCLUDE_ASAN_MESSAGES_H

static const char *const kAsanModuleCtorName = "asan.module_ctor";
static const char *const kAsanModuleDtorName = "asan.module_dtor";

static const char *const kAsanInitName = "__asan_init";
static const char *const kAsanHandleNoReturnName = "__asan_handle_no_return";

static const char *const kAsanRegisterGlobalsName = "__asan_register_globals";
static const char *const kAsanUnregisterGlobalsName = "__asan_unregister_globals";

static const char *const kAsanSetShadowPrefix = "__asan_set_shadow_";
static const char *const kAsanReportErrorTemplate = "__asan_report_";

static const char *const kAsanShadowMemoryDynamicAddress =
        "__asan_shadow_memory_dynamic_address";

static const char *const kAsanAllocaPoison = "__asan_alloca_poison";
static const char *const kAsanAllocasUnpoison = "__asan_allocas_unpoison";

#endif //MAPLE_SAN_INCLUDE__ASAN_MESSAGES_H
