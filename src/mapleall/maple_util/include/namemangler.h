/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef NAMEMANGLER_H
#define NAMEMANGLER_H
#include <string>
#include <vector>

// This is a general name mangler which is shared between maple compiler and runtime.
// maple-compiler-specific data structure may NOT be used here.
namespace namemangler {
#define TO_STR(s)  TO_STR2(s)
#define TO_STR2(s) #s


#define VTAB_PREFIX               __vtb_
#define ITAB_PREFIX               __itb_
#define VTAB_AND_ITAB_PREFIX      __vtb_and_itb_
#define ITAB_CONFLICT_PREFIX      __itbC_
#define CLASSINFO_PREFIX          __cinf_
#define CLASSINFO_RO_PREFIX       __classinforo__
#define SUPERCLASSINFO_PREFIX     __superclasses__
#define PRIMITIVECLASSINFO_PREFIX __pinf_
#define CLASS_INIT_BRIDGE_PREFIX  __ClassInitBridge__
#define GCTIB_PREFIX              MCC_GCTIB__
#define REF_PREFIX                REF_
#define JARRAY_PREFIX             A

#define VTAB_PREFIX_STR               TO_STR(VTAB_PREFIX)
#define ITAB_PREFIX_STR               TO_STR(ITAB_PREFIX)
#define VTAB_AND_ITAB_PREFIX_STR      TO_STR(VTAB_AND_ITAB_PREFIX)
#define ITAB_CONFLICT_PREFIX_STR      TO_STR(ITAB_CONFLICT_PREFIX)
#define CLASSINFO_PREFIX_STR          TO_STR(CLASSINFO_PREFIX)
#define CLASSINFO_RO_PREFIX_STR       TO_STR(CLASSINFO_RO_PREFIX)
#define SUPERCLASSINFO_PREFIX_STR     TO_STR(SUPERCLASSINFO_PREFIX)
#define PRIMITIVECLASSINFO_PREFIX_STR TO_STR(PRIMITIVECLASSINFO_PREFIX)
#define CLASS_INIT_BRIDGE_PREFIX_STR  TO_STR(CLASS_INIT_BRIDGE_PREFIX)
#define GCTIB_PREFIX_STR              TO_STR(GCTIB_PREFIX)
#define REF_PREFIX_STR                TO_STR(REF_PREFIX)
#define JARRAY_PREFIX_STR             TO_STR(JARRAY_PREFIX)

// Names of all compiler-generated tables and accessed by runtime
constexpr const char kMuidPrefixStr[] = "__muid_";
constexpr const char kMuidRoPrefixStr[] = "__muid_ro";
constexpr const char kMuidFuncDefTabPrefixStr[] = "__muid_func_def_tab";
constexpr const char kMuidFuncDefOrigTabPrefixStr[] = "__muid_ro_func_def_orig_tab";
constexpr const char kMuidFuncInfTabPrefixStr[] = "__muid_ro_func_inf_tab";
constexpr const char kMuidFuncMuidIdxTabPrefixStr[] = "__muid_ro_func_muid_idx_tab";
constexpr const char kMuidDataDefTabPrefixStr[] = "__muid_data_def_tab";
constexpr const char kMuidDataDefOrigTabPrefixStr[] = "__muid_ro_data_def_orig_tab";
constexpr const char kMuidFuncUndefTabPrefixStr[] = "__muid_func_undef_tab";
constexpr const char kMuidDataUndefTabPrefixStr[] = "__muid_data_undef_tab";
constexpr const char kMuidFuncDefMuidTabPrefixStr[] = "__muid_ro_func_def_muid_tab";
constexpr const char kMuidDataDefMuidTabPrefixStr[] = "__muid_ro_data_def_muid_tab";
constexpr const char kMuidFuncUndefMuidTabPrefixStr[] = "__muid_ro_func_undef_muid_tab";
constexpr const char kMuidDataUndefMuidTabPrefixStr[] = "__muid_ro_data_undef_muid_tab";
constexpr const char kMuidVtabAndItabPrefixStr[] = "__muid_vtab_and_itab";
constexpr const char kMuidItabConflictPrefixStr[] = "__muid_itab_conflict";
constexpr const char kMuidColdVtabAndItabPrefixStr[] = "__muid_cold_vtab_and_itab";
constexpr const char kMuidColdItabConflictPrefixStr[] = "__muid_cold_itab_conflict";
constexpr const char kMuidVtabOffsetPrefixStr[] = "__muid_vtab_offset_tab";
constexpr const char kMuidFieldOffsetPrefixStr[] = "__muid_field_offset_tab";
constexpr const char kMuidVtabOffsetKeyPrefixStr[] = "__muid_vtable_offset_key_tab";
constexpr const char kMuidFieldOffsetKeyPrefixStr[] = "__muid_field_offset_key_tab";
constexpr const char kMuidValueOffsetPrefixStr[] = "__muid_offset_value_table";
constexpr const char kMuidLocalClassInfoStr[] = "__muid_local_classinfo_tab";
constexpr const char kMuidSuperclassPrefixStr[] = "__muid_superclass";
constexpr const char kMuidGlobalRootlistPrefixStr[] = "__muid_globalrootlist";
constexpr const char kMuidClassMetadataPrefixStr[] = "__muid_classmetadata";
constexpr const char kMuidClassMetadataBucketPrefixStr[] = "__muid_classmetadata_bucket";
constexpr const char kMuidJavatextPrefixStr[] = "java_text";
constexpr const char kMuidDataSectionStr[] = "__data_section";
constexpr const char kMuidRangeTabPrefixStr[] = "__muid_range_tab";
constexpr const char kMuidConststrPrefixStr[] = "__muid_conststr";
constexpr const char kVtabOffsetTabStr[] = "__vtable_offset_table";
constexpr const char kFieldOffsetTabKeyStr[] = "__field_offset_key_table";
constexpr const char kFieldOffsetTabStr[] = "__field_offset_table";
constexpr const char kVtableKeyOffsetTabStr[] = "__vtable_offset_key_table";
constexpr const char kVtableOffsetTabKeyStr[] = "__vtable_offset_key_table";
constexpr const char kFieldKeyOffsetTabStr[] = "__field_offset_table";
constexpr const char kOffsetTabStr[] = "__offset_value_table";
constexpr const char kInlineCacheTabStr[] = "__inline_cache_table";
constexpr const char kLocalClassInfoStr[] = "__local_classinfo_table";
constexpr const char kMethodsInfoPrefixStr[] = "__methods_info__";
constexpr const char kMethodsInfoCompactPrefixStr[] = "__methods_infocompact__";
constexpr const char kFieldsInfoPrefixStr[] = "__fields_info__";
constexpr const char kFieldsInfoCompactPrefixStr[] = "__fields_infocompact__";
constexpr const char kFieldOffsetDataPrefixStr[] = "__fieldOffsetData__";
constexpr const char kMethodAddrDataPrefixStr[] = "__methodAddrData__";
constexpr const char kMethodSignaturePrefixStr[] = "__methodSignature__";
constexpr const char kParameterTypesPrefixStr[] = "__parameterTypes__";
constexpr const char kRegJNITabPrefixStr[] = "__reg_jni_tab";
constexpr const char kRegJNIFuncTabPrefixStr[] = "__reg_jni_func_tab";
constexpr const char kReflectionStrtabPrefixStr[] = "__reflection_strtab";
constexpr const char kReflectionStartHotStrtabPrefixStr[] = "__reflection_start_hot_strtab";
constexpr const char kReflectionBothHotStrTabPrefixStr[] = "__reflection_both_hot_strtab";
constexpr const char kReflectionRunHotStrtabPrefixStr[] = "__reflection_run_hot_strtab";
constexpr const char kReflectionNoEmitStrtabPrefixStr[] = "__reflection_no_emit_strtab";
constexpr const char kMarkMuidFuncDefStr[] = "muid_func_def:";
constexpr const char kMarkMuidFuncUndefStr[] = "muid_func_undef:";
constexpr const char kGcRootList[] = "gcRootNewList";
constexpr const char kDecoupleOption[] = "__decouple_option";
constexpr const char kDecoupleStr[] = "__decouple";
constexpr const char kCompilerVersionNum[] = "__compilerVersionNum";
constexpr const char kCompilerVersionNumStr[] = "__compilerVersionNumTab";
constexpr const char kCompilerMfileStatus[]  = "__compiler_mfile_status";
constexpr const char kMapleGlobalVariable[]  = "maple_global_variable";
constexpr const char kMapleLiteralString[]  = "maple_literal_string";

constexpr const char kSourceMuid[] = "__sourceMuid";
constexpr const char kSourceMuidSectionStr[] = "__sourceMuidTab";
constexpr const char kDecoupleStaticKeyStr[] = "__staticDecoupleKeyOffset";
constexpr const char kDecoupleStaticValueStr[] = "__staticDecoupleValueOffset";
constexpr const char kMarkDecoupleStaticStr[] = "decouple_static:";
constexpr const char kClassInfoPrefix[] = "__cinf";
constexpr const char kBssSectionStr[] = "__bss_section";
constexpr const char kLinkerHashSoStr[] = "__linkerHashSo";

constexpr const char kStaticFieldNamePrefixStr[] = "__static_field_name";
constexpr const char kMplSuffix[] = ".mpl";
constexpr const char kClinvocation[] = ".clinvocation";
constexpr const char kPackageNameSplitterStr[] = "_2F";
constexpr const char kFileNameSplitterStr[] = "$$";
constexpr const char kNameSplitterStr[] = "_7C";  // 7C is the ascii code for |
constexpr const char kRightBracketStr[] = "_29";  // 29 is the ascii code for )
constexpr const char kClassNameSplitterStr[] = "_3B_7C";
constexpr const char kJavaLangClassStr[] = "Ljava_2Flang_2FClass_3B";
constexpr const char kJavaLangObjectStr[] = "Ljava_2Flang_2FObject_3B";
constexpr const char kJavaLangClassloader[] = "Ljava_2Flang_2FClassLoader_3B";
constexpr const char kJavaLangObjectStrJVersion[] = "Ljava/lang/Object;";
constexpr const char kJavaLangStringStr[] = "Ljava_2Flang_2FString_3B";
constexpr const char kJavaLangExceptionStr[] = "Ljava_2Flang_2FException_3B";
constexpr const char kThrowClassStr[] = "Ljava_2Flang_2FThrowable_3B";
constexpr const char kReflectionClassesPrefixStr[] = "Ljava_2Flang_2Freflect_2F";
constexpr const char kReflectionClassMethodStr[] = "Ljava_2Flang_2Freflect_2FMethod_241_3B";
constexpr const char kClassMetadataTypeName[] = "__class_meta__";
constexpr const char kPtrPrefixStr[] = "_PTR";
constexpr const char kClassINfoPtrPrefixStr[] = "_PTR__cinf_";
constexpr const char kArrayClassInfoPrefixStr[] = "__cinf_A";
constexpr const char kShadowClassName[] = "shadow_24__klass__";
constexpr const char kClinitSuffix[] = "_7C_3Cclinit_3E_7C_28_29V";
constexpr const char kCinitStr[] = "_7C_3Cinit_3E_7C_28";
constexpr const char kClinitSubStr[] = "7C_3Cinit_3E_7C";

constexpr const char kPreNativeFunc[] = "MCC_PreNativeCall";
constexpr const char kPostNativeFunc[] = "MCC_PostNativeCall";
constexpr const char kDecodeRefFunc[] = "MCC_DecodeReference";
constexpr const char kFindNativeFunc[] = "MCC_FindNativeMethodPtr";
constexpr const char kFindNativeFuncNoeh[] = "MCC_FindNativeMethodPtrWithoutException";
constexpr const char kDummyNativeFunc[] = "MCC_DummyNativeMethodPtr";
constexpr const char kCheckThrowPendingExceptionFunc[] = "MCC_CheckThrowPendingException";
constexpr const char kCallFastNative[] = "MCC_CallFastNative";
constexpr const char kCallFastNativeExt[] = "MCC_CallFastNativeExt";
constexpr const char kCallSlowNativeExt[] = "MCC_CallSlowNativeExt";
constexpr const char kSetReliableUnwindContextFunc[] = "MCC_SetReliableUnwindContext";

constexpr const char kArrayClassCacheTable[] = "__arrayClassCacheTable";
constexpr const char kArrayClassCacheNameTable[] = "__muid_ro_arrayClassCacheNameTable";
constexpr const char kFunctionLayoutStr[] = "__func_layout__";

constexpr const char kFunctionProfileTabPrefixStr[] = "__muid_profile_func_tab";

constexpr const char kBBProfileTabPrefixStr[] = "__muid_prof_counter_tab";
constexpr const char kFuncIRProfInfTabPrefixStr[] = "__muid_prof_ir_desc_tab";

constexpr const char kprefixProfModDesc[] = "__mpl_prof_moddesc_";
constexpr const char kprefixProfCtrTbl[] = "__mpl_prof_ctrtbl_";
constexpr const char kprefixProfFuncDesc[] = "__mpl_prof_funcdesc_";
constexpr const char kprefixProfFuncDescTbl[] = "__mpl_func_prof_desc_tbl_";
constexpr const char kprefixProfInit[] = "__mpl_prof_init_";
constexpr const char kprefixProfExit[] = "__mpl_prof_exit_";
constexpr const char kGCCProfInit[] = "__gcov_init";
constexpr const char kGCCProfExit[] = "__gcov_exit";
constexpr const char kMplMergeFuncAdd[] = "__gcov_merge_add";
constexpr const char kProfFileNameExt[] = ".gcda";

constexpr const char kBindingProtectedRegionStr[] = "__BindingProtectRegion__";

constexpr const char kClassNamePrefixStr[] = "L";
constexpr const char kClassMethodSplitterStr[] = "_3B";
constexpr const char kFuncGetCurrentCl[] = "MCC_GetCurrentClassLoader";
static constexpr const char kMplProfFileNameExt[] = ".mprofdata";
// Serve as a global flag to indicate whether frequent strings have been compressed
extern bool doCompression;

// Return the input string if the compression is not on; otherwise, return its compressed version
std::string GetInternalNameLiteral(std::string name);
std::string GetOriginalNameLiteral(std::string name);

std::string EncodeName(const std::string &name);
std::string DecodeName(const std::string &name);
void DecodeMapleNameToJavaDescriptor(const std::string &nameIn, std::string &nameOut);

std::string NativeJavaName(const std::string &name, bool overLoaded = true);

__attribute__((visibility("default"))) unsigned UTF16ToUTF8(std::string &str, const std::u16string &str16,
                                                            unsigned short num = 0, bool isBigEndian = false);
__attribute__((visibility("default"))) unsigned UTF8ToUTF16(std::u16string &str16, const std::string &str8,
                                                            unsigned short num = 0, bool isBigEndian = false);
void GetUnsignedLeb128Encode(std::vector<uint8_t> &dest, uint32_t value);
uint32_t GetUnsignedLeb128Decode(const uint8_t **data);
size_t GetUleb128Size(uint64_t v);
size_t GetSleb128Size(int32_t v);
bool NeedConvertUTF16(const std::string &str8);
uint32_t EncodeSLEB128(uint64_t value, std::ofstream &out);
uint32_t EncodeULEB128(uint64_t value, std::ofstream &out);
uint64_t DecodeULEB128(const uint8_t *p, unsigned *n = nullptr, const uint8_t *end = nullptr);
int64_t DecodeSLEB128(const uint8_t *p, unsigned *n = nullptr, const uint8_t *end = nullptr);
constexpr int kZeroAsciiNum = 48;
constexpr int kAAsciiNum = 65;
constexpr int kaAsciiNum = 97;
} // namespace namemangler

#endif
