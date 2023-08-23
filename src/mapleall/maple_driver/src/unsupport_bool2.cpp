/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "driver_options.h"

namespace opts {


maplecl::Option<bool> fpie({"-fpie", "--fpie"},
    "  --fpie                      \tGenerate position-independent executable in small mode.\n"
    "  --no-pie/-fno-pie           \n",
    {cgCategory, driverCategory, ldCategory}, kOptCommon | kOptLd | kOptNotFiltering,
    maplecl::DisableEvery({"-fno-pie", "--no-pie"}));

maplecl::Option<bool> oFipaSra({"-fipa-sra"},
    "  -fipa-sra                   \tPerform interprocedural reduction of aggregates.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-sra"), maplecl::kHide);

maplecl::Option<bool> oFiraHoistPressure({"-fira-hoist-pressure"},
    "  -fira-hoist-pressure        \tUse IRA based register pressure calculation in RTL hoist optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ira-hoist-pressure"), maplecl::kHide);

maplecl::Option<bool> oFiraLoopPressure({"-fira-loop-pressure"},
    "  -fira-loop-pressure         \tUse IRA based register pressure calculation in RTL loop optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ira-loop-pressure"), maplecl::kHide);

maplecl::Option<bool> oFisolateErroneousPathsAttribute({"-fisolate-erroneous-paths-attribute"},
    "  -fisolate-erroneous-paths-attribute  \tDetect paths that trigger erroneous or undefined behavior due to a "
    "null value being used in a way forbidden by a returns_nonnull or nonnull attribute.  Isolate those paths from "
    "the main control flow and turn the statement with erroneous or undefined behavior into a trap.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-isolate-erroneous-paths-attribute"), maplecl::kHide);

maplecl::Option<bool> oFisolateErroneousPathsDereference({"-fisolate-erroneous-paths-dereference"},
    "  -fisolate-erroneous-paths-dereference \tDetect paths that trigger erroneous or undefined behavior due to "
    "dereferencing a null pointer.  Isolate those paths from the main control flow and turn the statement with "
    "erroneous or undefined behavior into a trap.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-isolate-erroneous-paths-dereference"), maplecl::kHide);

maplecl::Option<bool> oFivopts({"-fivopts"},
    "  -fivopts                    \tOptimize induction variables on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ivopts"), maplecl::kHide);

maplecl::Option<bool> oFkeepInlineFunctions({"-fkeep-inline-functions"},
    "  -fkeep-inline-functions     \tGenerate code for functions even if they are fully inlined.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-keep-inline-functions"), maplecl::kHide);

maplecl::Option<bool> oFkeepStaticConsts({"-fkeep-static-consts"},
    "  -fkeep-static-consts        \tEmit static const variables even if they are not used.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-keep-static-consts"), maplecl::kHide);

maplecl::Option<bool> oFkeepStaticFunctions({"-fkeep-static-functions"},
    "  -fkeep-static-functions             \tGenerate code for static functions even if they are never called.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-keep-static-functions"), maplecl::kHide);

maplecl::Option<bool> oFlat_namespace({"-flat_namespace"},
    "  -flat_namespace             \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lat_namespace"), maplecl::kHide);

maplecl::Option<bool> oFlaxVectorConversions({"-flax-vector-conversions"},
    "  -flax-vector-conversions    \tAllow implicit conversions between vectors with differing numbers of subparts "
    "and/or differing element types.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lax-vector-conversions"), maplecl::kHide);

maplecl::Option<bool> oFleadingUnderscore({"-fleading-underscore"},
    "  -fleading-underscore        \tGive external symbols a leading underscore.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-leading-underscore"), maplecl::kHide);

maplecl::Option<bool> oFlocalIvars({"-flocal-ivars"},
    "  -flocal-ivars               \tAllow access to instance variables as if they were local declarations within "
    "instance method implementations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-local-ivars"), maplecl::kHide);

maplecl::Option<bool> oFloopBlock({"-floop-block"},
    "  -floop-block                \tEnable loop nest transforms.  Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-loop-block"), maplecl::kHide);

maplecl::Option<bool> oFloopInterchange({"-floop-interchange"},
    "  -floop-interchange          \tEnable loop nest transforms.  Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-loop-interchange"), maplecl::kHide);

maplecl::Option<bool> oFloopNestOptimize({"-floop-nest-optimize"},
    "  -floop-nest-optimize        \tEnable the loop nest optimizer.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-loop-nest-optimize"), maplecl::kHide);

maplecl::Option<bool> oFloopParallelizeAll({"-floop-parallelize-all"},
    "  -floop-parallelize-all      \tMark all loops as parallel.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-loop-parallelize-all"), maplecl::kHide);

maplecl::Option<bool> oFloopStripMine({"-floop-strip-mine"},
    "  -floop-strip-mine           \tEnable loop nest transforms. Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-loop-strip-mine"), maplecl::kHide);

maplecl::Option<bool> oFloopUnrollAndJam({"-floop-unroll-and-jam"},
    "  -floop-unroll-and-jam       \tEnable loop nest transforms. Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-loop-unroll-and-jam"), maplecl::kHide);

maplecl::Option<bool> oFlraRemat({"-flra-remat"},
    "  -flra-remat                 \tDo CFG-sensitive rematerialization in LRA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lra-remat"), maplecl::kHide);

maplecl::Option<bool> oFltoOdrTypeMerging({"-flto-odr-type-merging"},
    "  -flto-odr-type-merging      \tMerge C++ types using One Definition Rule.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lto-odr-type-merging"), maplecl::kHide);

maplecl::Option<bool> oFltoReport({"-flto-report"},
    "  -flto-report                \tReport various link-time optimization statistics.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lto-report"), maplecl::kHide);

maplecl::Option<bool> oFltoReportWpa({"-flto-report-wpa"},
    "  -flto-report-wpa            \tReport various link-time optimization statistics for WPA only.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lto-report-wpa"), maplecl::kHide);

maplecl::Option<bool> oFmemReport({"-fmem-report"},
    "  -fmem-report                \tReport on permanent memory allocation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-mem-report"), maplecl::kHide);

maplecl::Option<bool> oFmemReportWpa({"-fmem-report-wpa"},
    "  -fmem-report-wpa            \tReport on permanent memory allocation in WPA only.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-mem-report-wpa"), maplecl::kHide);

maplecl::Option<bool> oFmergeAllConstants({"-fmerge-all-constants"},
    "  -fmerge-all-constants       \tAttempt to merge identical constants and constantvariables.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-merge-all-constants"), maplecl::kHide);

maplecl::Option<bool> oFmergeConstants({"-fmerge-constants"},
    "  -fmerge-constants           \tAttempt to merge identical constants across compilation units.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-merge-constants"), maplecl::kHide);

maplecl::Option<bool> oFmergeDebugStrings({"-fmerge-debug-strings"},
    "  -fmerge-debug-strings       \tAttempt to merge identical debug strings across compilation units.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-merge-debug-strings"), maplecl::kHide);

maplecl::Option<bool> oFmoduloSched({"-fmodulo-sched"},
    "  -fmodulo-sched              \tPerform SMS based modulo scheduling before the first scheduling pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-modulo-sched"), maplecl::kHide);

maplecl::Option<bool> oFmoduloSchedAllowRegmoves({"-fmodulo-sched-allow-regmoves"},
    "  -fmodulo-sched-allow-regmoves  \tPerform SMS based modulo scheduling with register moves allowed.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-modulo-sched-allow-regmoves"), maplecl::kHide);

maplecl::Option<bool> oFmoveLoopInvariants({"-fmove-loop-invariants"},
    "  -fmove-loop-invariants      \tMove loop invariant computations out of loops.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-move-loop-invariants"), maplecl::kHide);

maplecl::Option<bool> oFmsExtensions({"-fms-extensions"},
    "  -fms-extensions             \tDon't warn about uses of Microsoft extensions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ms-extensions"), maplecl::kHide);

maplecl::Option<bool> oFnewInheritingCtors({"-fnew-inheriting-ctors"},
    "  -fnew-inheriting-ctors      \tImplement C++17 inheriting constructor semantics.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-new-inheriting-ctors"), maplecl::kHide);

maplecl::Option<bool> oFnewTtpMatching({"-fnew-ttp-matching"},
    "  -fnew-ttp-matching          \tImplement resolution of DR 150 for matching of template template arguments.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-new-ttp-matching"), maplecl::kHide);

maplecl::Option<bool> oFnextRuntime({"-fnext-runtime"},
    "  -fnext-runtime              \tGenerate code for NeXT (Apple Mac OS X) runtime environment.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFnoAccessControl({"-fno-access-control"},
    "  -fno-access-control         \tTurn off all access checking.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFnoAsm({"-fno-asm"},
    "  -fno-asm                    \tDo not recognize asm, inline or typeof as a keyword, so that code can use these "
    "words as identifiers. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFnoBranchCountReg({"-fno-branch-count-reg"},
    "  -fno-branch-count-reg       \tAvoid running a pass scanning for opportunities to use \"decrement and branch\" "
    "instructions on a count register instead of generating sequences of instructions that decrement a register, "
    "compare it against zero, and then branch based upon the result.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFnoCanonicalSystemHeaders({"-fno-canonical-system-headers"},
    "  -fno-canonical-system-headers  \tWhen preprocessing, do not shorten system header paths with "
    "canonicalization.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFCheckPointerBounds({"-fcheck-pointer-bounds"},
    "  -fcheck-pointer-bounds      \tEnable Pointer Bounds Checker instrumentation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-check-pointer-bounds"), maplecl::kHide);

maplecl::Option<bool> oFChecking({"-fchecking"},
    "  -fchecking                  \tPerform internal consistency checkings.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-checking"), maplecl::kHide);

maplecl::Option<bool> oFChkpCheckIncompleteType({"-fchkp-check-incomplete-type"},
    "  -fchkp-check-incomplete-type  \tGenerate pointer bounds checks for variables with incomplete type.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-check-incomplete-type"), maplecl::kHide);

maplecl::Option<bool> oFChkpCheckRead({"-fchkp-check-read"},
    "  -fchkp-check-read           \tGenerate checks for all read accesses to memory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-check-read"), maplecl::kHide);

maplecl::Option<bool> oFChkpCheckWrite({"-fchkp-check-write"},
    "  -fchkp-check-write          \tGenerate checks for all write accesses to memory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-check-write"), maplecl::kHide);

maplecl::Option<bool> oFChkpFirstFieldHasOwnBounds({"-fchkp-first-field-has-own-bounds"},
    "  -fchkp-first-field-has-own-bounds  \tForces Pointer Bounds Checker to use narrowed bounds for address of "
    "the first field in the structure.  By default pointer to the first field has the same bounds as pointer to the "
    "whole structure.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-first-field-has-own-bounds"), maplecl::kHide);

maplecl::Option<bool> oFDefaultInline({"-fdefault-inline"},
    "  -fdefault-inline            \tDoes nothing.  Preserved for backward compatibility.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-default-inline"), maplecl::kHide);

maplecl::Option<bool> oFdefaultInteger8({"-fdefault-integer-8"},
    "  -fdefault-integer-8         \tSet the default integer kind to an 8 byte wide type.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-default-integer-8"), maplecl::kHide);

maplecl::Option<bool> oFdefaultReal8({"-fdefault-real-8"},
    "  -fdefault-real-8            \tSet the default real kind to an 8 byte wide type.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-default-real-8"), maplecl::kHide);

maplecl::Option<bool> oFDeferPop({"-fdefer-pop"},
    "  -fdefer-pop                 \tDefer popping functions args from stack until later.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-defer-pop"), maplecl::kHide);

maplecl::Option<bool> oFElideConstructors({"-felide-constructors"},
    "  -felide-constructors        \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("fno-elide-constructors"), maplecl::kHide);

maplecl::Option<bool> oFEnforceEhSpecs({"-fenforce-eh-specs"},
    "  -fenforce-eh-specs          \tGenerate code to check exception specifications.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-enforce-eh-specs"), maplecl::kHide);

maplecl::Option<bool> oFFpIntBuiltinInexact({"-ffp-int-builtin-inexact"},
    "  -ffp-int-builtin-inexact    \tAllow built-in functions ceil, floor, round, trunc to raise 'inexact' "
    "exceptions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fp-int-builtin-inexact"), maplecl::kHide);

maplecl::Option<bool> oFFunctionCse({"-ffunction-cse"},
    "  -ffunction-cse              \tAllow function addresses to be held in registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-function-cse"), maplecl::kHide);

maplecl::Option<bool> oFGnuKeywords({"-fgnu-keywords"},
    "  -fgnu-keywords              \tRecognize GNU-defined keywords.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gnu-keywords"), maplecl::kHide);

maplecl::Option<bool> oFGnuUnique({"-fgnu-unique"},
    "  -fgnu-unique                \tUse STB_GNU_UNIQUE if supported by the assembler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gnu-unique"), maplecl::kHide);

maplecl::Option<bool> oFGuessBranchProbability({"-fguess-branch-probability"},
    "  -fguess-branch-probability  \tEnable guessing of branch probabilities.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-guess-branch-probability"), maplecl::kHide);

maplecl::Option<bool> oFIdent({"-fident"},
    "  -fident                     \tProcess #ident directives.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ident"), maplecl::kHide);

maplecl::Option<bool> oFImplementInlines({"-fimplement-inlines"},
    "  -fimplement-inlines         \tExport functions even if they can be inlined.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-implement-inlines"), maplecl::kHide);

maplecl::Option<bool> oFImplicitInlineTemplates({"-fimplicit-inline-templates"},
    "  -fimplicit-inline-templates \tEmit implicit instantiations of inline templates.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-implicit-inline-templates"), maplecl::kHide);

maplecl::Option<bool> oFImplicitTemplates({"-fimplicit-templates"},
    "  -fimplicit-templates        \tEmit implicit instantiations of templates.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("no-implicit-templates"), maplecl::kHide);

maplecl::Option<bool> oFIraShareSaveSlots({"-fira-share-save-slots"},
    "  -fira-share-save-slots      \tShare slots for saving different hard registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ira-share-save-slots"), maplecl::kHide);

maplecl::Option<bool> oFIraShareSpillSlots({"-fira-share-spill-slots"},
    "  -fira-share-spill-slots      \tShare stack slots for spilled pseudo-registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ira-share-spill-slots"), maplecl::kHide);

maplecl::Option<bool> oFJumpTables({"-fjump-tables"},
    "  -fjump-tables               \tUse jump tables for sufficiently large switch statements.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-jump-tables"), maplecl::kHide);

maplecl::Option<bool> oFKeepInlineDllexport({"-fkeep-inline-dllexport"},
    "  -fkeep-inline-dllexport     \tDon't emit dllexported inline functions unless needed.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-keep-inline-dllexport"), maplecl::kHide);

maplecl::Option<bool> oFLifetimeDse({"-flifetime-dse"},
    "  -flifetime-dse              \tTell DSE that the storage for a C++ object is dead when the constructor"
    " starts and when the destructor finishes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-lifetime-dse"), maplecl::kHide);

maplecl::Option<bool> oFMathErrno({"-fmath-errno"},
    "  -fmath-errno                \tSet errno after built-in math functions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-math-errno"), maplecl::kHide);

maplecl::Option<bool> oFNilReceivers({"-fnil-receivers"},
    "  -fnil-receivers             \tAssume that receivers of Objective-C messages may be nil.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-nil-receivers"), maplecl::kHide);

maplecl::Option<bool> oFNonansiBuiltins({"-fnonansi-builtins"},
    "  -fnonansi-builtins          \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-nonansi-builtins"), maplecl::kHide);

maplecl::Option<bool> oFOperatorNames({"-foperator-names"},
    "  -foperator-names            \tRecognize C++ keywords like 'compl' and 'xor'.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-operator-names"), maplecl::kHide);

maplecl::Option<bool> oFOptionalDiags({"-foptional-diags"},
    "  -foptional-diags            \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-optional-diags"), maplecl::kHide);

maplecl::Option<bool> oFPeephole({"-fpeephole"},
    "  -fpeephole                  \tEnable machine specific peephole optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-peephole"), maplecl::kHide);

maplecl::Option<bool> oFPeephole2({"-fpeephole2"},
    "  -fpeephole2                 \tEnable an RTL peephole pass before sched2.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-peephole2"), maplecl::kHide);

maplecl::Option<bool> oFPrettyTemplates({"-fpretty-templates"},
    "  -fpretty-templates          \tpretty-print template specializations as the template signature followed "
    "by the arguments.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-pretty-templates"), maplecl::kHide);

maplecl::Option<bool> oFPrintfReturnValue({"-fprintf-return-value"},
    "  -fprintf-return-value       \tTreat known sprintf return values as constants.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-printf-return-value"), maplecl::kHide);

maplecl::Option<bool> oFRtti({"-frtti"},
    "  -frtti                      \tGenerate run time type descriptor information.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-rtti"), maplecl::kHide);

maplecl::Option<bool> oFnoSanitizeAll({"-fno-sanitize=all"},
    "  -fno-sanitize=all           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFSchedInterblock({"-fsched-interblock"},
    "  -fsched-interblock          \tEnable scheduling across basic blocks.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-interblock"), maplecl::kHide);

maplecl::Option<bool> oFSchedSpec({"-fsched-spec"},
    "  -fsched-spec                \tAllow speculative motion of non-loads.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-spec"), maplecl::kHide);

maplecl::Option<bool> oFnoSetStackExecutable({"-fno-set-stack-executable"},
    "  -fno-set-stack-executable   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFShowColumn({"-fshow-column"},
    "  -fshow-column               \tShow column numbers in diagnostics, when available.  Default on.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-show-column"), maplecl::kHide);

maplecl::Option<bool> oFSignedZeros({"-fsigned-zeros"},
    "  -fsigned-zeros              \tDisable floating point optimizations that ignore the IEEE signedness of zero.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-signed-zeros"), maplecl::kHide);

maplecl::Option<bool> oFStackLimit({"-fstack-limit"},
    "  -fstack-limit               \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stack-limit"), maplecl::kHide);

maplecl::Option<bool> oFThreadsafeStatics({"-fthreadsafe-statics"},
    "  -fthreadsafe-statics        \tDo not generate thread-safe code for initializing local statics.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-threadsafe-statics"), maplecl::kHide);

maplecl::Option<bool> oFToplevelReorder({"-ftoplevel-reorder"},
    "  -ftoplevel-reorder         \tReorder top level functions, variables, and asms.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-toplevel-reorder"), maplecl::kHide);

maplecl::Option<bool> oFTrappingMath({"-ftrapping-math"},
    "  -ftrapping-math             \tAssume floating-point operations can trap.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-trapping-math"), maplecl::kHide);

maplecl::Option<bool> oFUseCxaGetExceptionPtr({"-fuse-cxa-get-exception-ptr"},
    "  -fuse-cxa-get-exception-ptr \tUse __cxa_get_exception_ptr in exception handling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-use-cxa-get-exception-ptr"), maplecl::kHide);

maplecl::Option<bool> oFWeak({"-fweak"},
    "  -fweak                      \tEmit common-like symbols as weak symbols.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-weak"), maplecl::kHide);

maplecl::Option<bool> oFnoWritableRelocatedRdata({"-fno-writable-relocated-rdata"},
    "  -fno-writable-relocated-rdata  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFZeroInitializedInBss({"-fzero-initialized-in-bss"},
    "  -fzero-initialized-in-bss   \tPut zero initialized data in the bss section.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-zero-initialized-in-bss"), maplecl::kHide);

maplecl::Option<bool> oFnonCallExceptions({"-fnon-call-exceptions"},
    "  -fnon-call-exceptions       \tSupport synchronous non-call exceptions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-non-call-exceptions"), maplecl::kHide);

maplecl::Option<bool> oFnothrowOpt({"-fnothrow-opt"},
    "  -fnothrow-opt               \tTreat a throw() exception specification as noexcept to improve code size.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-nothrow-opt"), maplecl::kHide);

maplecl::Option<bool> oFobjcCallCxxCdtors({"-fobjc-call-cxx-cdtors"},
    "  -fobjc-call-cxx-cdtors      \tGenerate special Objective-C methods to initialize/destroy non-POD C++ ivars\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-objc-call-cxx-cdtors"), maplecl::kHide);

maplecl::Option<bool> oFobjcDirectDispatch({"-fobjc-direct-dispatch"},
    "  -fobjc-direct-dispatch      \tAllow fast jumps to the message dispatcher.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-objc-direct-dispatch"), maplecl::kHide);

maplecl::Option<bool> oFobjcExceptions({"-fobjc-exceptions"},
    "  -fobjc-exceptions           \tEnable Objective-C exception and synchronization syntax.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-objc-exceptions"), maplecl::kHide);

maplecl::Option<bool> oFobjcGc({"-fobjc-gc"},
    "  -fobjc-gc                   \tEnable garbage collection (GC) in Objective-C/Objective-C++ programs.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-objc-gc"), maplecl::kHide);

maplecl::Option<bool> oFobjcNilcheck({"-fobjc-nilcheck"},
    "  -fobjc-nilcheck             \tEnable inline checks for nil receivers with the NeXT runtime and ABI version 2.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fobjc-nilcheck"), maplecl::kHide);

maplecl::Option<bool> oFobjcSjljExceptions({"-fobjc-sjlj-exceptions"},
    "  -fobjc-sjlj-exceptions      \tEnable Objective-C setjmp exception handling runtime.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-objc-sjlj-exceptions"), maplecl::kHide);

maplecl::Option<bool> oFobjcStd({"-fobjc-std=objc1"},
    "  -fobjc-std                  \tConform to the Objective-C 1.0 language as implemented in GCC 4.0.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFopenacc({"-fopenacc"},
    "  -fopenacc                   \tEnable OpenACC.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-openacc"), maplecl::kHide);

maplecl::Option<bool> oFopenmp({"-fopenmp"},
    "  -fopenmp                    \tEnable OpenMP (implies -frecursive in Fortran).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-openmp"), maplecl::kHide);

maplecl::Option<bool> oFopenmpSimd({"-fopenmp-simd"},
    "  -fopenmp-simd               \tEnable OpenMP's SIMD directives.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-openmp-simd"), maplecl::kHide);

maplecl::Option<bool> oFoptInfo({"-fopt-info"},
    "  -fopt-info                  \tEnable all optimization info dumps on stderr.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-opt-info"), maplecl::kHide);

maplecl::Option<bool> oFoptimizeStrlen({"-foptimize-strlen"},
    "  -foptimize-strlen           \tEnable string length optimizations on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-foptimize-strlen"), maplecl::kHide);

maplecl::Option<bool> oForce_cpusubtype_ALL({"-force_cpusubtype_ALL"},
    "  -force_cpusubtype_ALL       \tThis causes GCC's output file to have the 'ALL' subtype, instead of one "
    "controlled by the -mcpu or -march option.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oForce_flat_namespace({"-force_flat_namespace"},
    "  -force_flat_namespace       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFpackStruct({"-fpack-struct"},
    "  -fpack-struct               \tPack structure members together without holes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-pack-struct"), maplecl::kHide);

maplecl::Option<bool> oFpartialInlining({"-fpartial-inlining"},
    "  -fpartial-inlining          \tPerform partial inlining.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-partial-inlining"), maplecl::kHide);

maplecl::Option<bool> oFpccStructReturn({"-fpcc-struct-return"},
    "  -fpcc-struct-return         \tReturn small aggregates in memory\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-pcc-struct-return"), maplecl::kHide);

maplecl::Option<bool> oFpchDeps({"-fpch-deps"},
    "  -fpch-deps                  \tWhen using precompiled headers (see Precompiled Headers), this flag causes "
    "the dependency-output flags to also list the files from the precompiled header's dependencies.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-pch-deps"), maplecl::kHide);

maplecl::Option<bool> oFnoPchPreprocess({"-fno-pch-preprocess"},
    "  -fno-pch-preprocess         \tLook for and use PCH files even when preprocessing.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFpeelLoops({"-fpeel-loops"},
    "  -fpeel-loops                \tPerform loop peeling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-peel-loops"), maplecl::kHide);

maplecl::Option<bool> oFpermissive({"-fpermissive"},
    "  -fpermissive                \tDowngrade conformance errors to warnings.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-permissive"), maplecl::kHide);

maplecl::Option<bool> oFplan9Extensions({"-fplan9-extensions"},
    "  -fplan9-extensions          \tEnable Plan 9 language extensions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fplan9-extensions"), maplecl::kHide);

maplecl::Option<bool> oFpostIpaMemReport({"-fpost-ipa-mem-report"},
    "  -fpost-ipa-mem-report       \tReport on memory allocation before interprocedural optimization.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-post-ipa-mem-report"), maplecl::kHide);

maplecl::Option<bool> oFpreIpaMemReport({"-fpre-ipa-mem-report"},
    "  -fpre-ipa-mem-report        \tReport on memory allocation before interprocedural optimization.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-pre-ipa-mem-report"), maplecl::kHide);

maplecl::Option<bool> oFpredictiveCommoning({"-fpredictive-commoning"},
    "  -fpredictive-commoning      \tRun predictive commoning optimization.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fpredictive-commoning"), maplecl::kHide);

maplecl::Option<bool> oFprefetchLoopArrays({"-fprefetch-loop-arrays"},
    "  -fprefetch-loop-arrays      \tGenerate prefetch instructions\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-prefetch-loop-arrays"), maplecl::kHide);

maplecl::Option<bool> oFpreprocessed({"-fpreprocessed"},
    "  -fpreprocessed              \tTreat the input file as already preprocessed.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-preprocessed"), maplecl::kHide);

maplecl::Option<bool> oFprofileArcs({"-fprofile-arcs"},
    "  -fprofile-arcs              \tInsert arc-based program profiling code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-arcs"), maplecl::kHide);

maplecl::Option<bool> oFprofileCorrection({"-fprofile-correction"},
    "  -fprofile-correction        \tEnable correction of flow inconsistent profile data input.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-correction"), maplecl::kHide);

maplecl::Option<bool> oFprofileGenerate({"-fprofile-generate"},
    "  -fprofile-generate          \tEnable common options for generating profile info for profile feedback directed "
    "optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-generate"), maplecl::kHide);

maplecl::Option<bool> oFprofileReorderFunctions({"-fprofile-reorder-functions"},
    "  -fprofile-reorder-functions \tEnable function reordering that improves code placement.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-reorder-functions"), maplecl::kHide);

maplecl::Option<bool> oFprofileReport({"-fprofile-report"},
    "  -fprofile-report            \tReport on consistency of profile.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-report"), maplecl::kHide);

maplecl::Option<bool> oFprofileUse({"-fprofile-use"},
    "  -fprofile-use               \tEnable common options for performing profile feedback directed optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-use"), maplecl::kHide);

maplecl::Option<bool> oFprofileValues({"-fprofile-values"},
    "  -fprofile-values            \tInsert code to profile values of expressions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-profile-values"), maplecl::kHide);

maplecl::Option<bool> oFpu({"-fpu"},
    "  -fpu                        \tEnables (-fpu) or disables (-nofpu) the use of RX floating-point hardware. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-nofpu"), maplecl::kHide);

maplecl::Option<bool> oFrandomSeed({"-frandom-seed"},
    "  -frandom-seed               \tMake compile reproducible using <string>.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-random-seed"), maplecl::kHide);

maplecl::Option<bool> oFreciprocalMath({"-freciprocal-math"},
    "  -freciprocal-math           \tSame as -fassociative-math for expressions which include division.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-reciprocal-math"), maplecl::kHide);

maplecl::Option<bool> oFrecordGccSwitches({"-frecord-gcc-switches"},
    "  -frecord-gcc-switches       \tRecord gcc command line switches in the object file.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-record-gcc-switches"), maplecl::kHide);

maplecl::Option<bool> oFree({"-free"},
    "  -free                       \tTurn on Redundant Extensions Elimination pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ree"), maplecl::kHide);

maplecl::Option<bool> oFrenameRegisters({"-frename-registers"},
    "  -frename-registers          \tPerform a register renaming optimization pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-rename-registers"), maplecl::kHide);

maplecl::Option<bool> oFreorderBlocks({"-freorder-blocks"},
    "  -freorder-blocks            \tReorder basic blocks to improve code placement.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-reorder-blocks"), maplecl::kHide);


maplecl::Option<bool> oFreorderBlocksAndPartition({"-freorder-blocks-and-partition"},
    "  -freorder-blocks-and-partition  \tReorder basic blocks and partition into hot and cold sections.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-reorder-blocks-and-partition"), maplecl::kHide);

maplecl::Option<bool> oFreorderFunctions({"-freorder-functions"},
    "  -freorder-functions         \tReorder functions to improve code placement.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-reorder-functions"), maplecl::kHide);

maplecl::Option<bool> oFreplaceObjcClasses({"-freplace-objc-classes"},
    "  -freplace-objc-classes      \tUsed in Fix-and-Continue mode to indicate that object files may be swapped in "
    "at runtime.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-replace-objc-classes"), maplecl::kHide);

maplecl::Option<bool> oFrepo({"-frepo"},
    "  -frepo                      \tEnable automatic template instantiation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-repo"), maplecl::kHide);

maplecl::Option<bool> oFreportBug({"-freport-bug"},
    "  -freport-bug                \tCollect and dump debug information into temporary file if ICE in C/C++ "
    "compiler occurred.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-report-bug"), maplecl::kHide);

maplecl::Option<bool> oFrerunCseAfterLoop({"-frerun-cse-after-loop"},
    "  -frerun-cse-after-loop      \tAdd a common subexpression elimination pass after loop optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-rerun-cse-after-loop"), maplecl::kHide);

maplecl::Option<bool> oFrescheduleModuloScheduledLoops({"-freschedule-modulo-scheduled-loops"},
    "  -freschedule-modulo-scheduled-loops  \tEnable/Disable the traditional scheduling in loops that already "
    "passed modulo scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-reschedule-modulo-scheduled-loops"), maplecl::kHide);

maplecl::Option<bool> oFroundingMath({"-frounding-math"},
    "  -frounding-math             \tDisable optimizations that assume default FP rounding behavior.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-rounding-math"), maplecl::kHide);

maplecl::Option<bool> oFsanitizeAddressUseAfterScope({"-fsanitize-address-use-after-scope"},
    "  -fsanitize-address-use-after-scope  \tEnable sanitization of local variables to detect use-after-scope bugs.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sanitize-address-use-after-scope"), maplecl::kHide);

maplecl::Option<bool> oFsanitizeCoverageTracePc({"-fsanitize-coverage=trace-pc"},
    "  -fsanitize-coverage=trace-pc  \tEnable coverage-guided fuzzing code instrumentation. Inserts call to "
    "__sanitizer_cov_trace_pc into every basic block.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sanitize-coverage=trace-pc"), maplecl::kHide);

maplecl::Option<bool> oFsanitizeRecover({"-fsanitize-recover"},
    "  -fsanitize-recover          \tAfter diagnosing undefined behavior attempt to continue execution.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sanitize-recover"), maplecl::kHide);

maplecl::Option<bool> oFsanitizeUndefinedTrapOnError({"-fsanitize-undefined-trap-on-error"},
    "  -fsanitize-undefined-trap-on-error  \tUse trap instead of a library function for undefined behavior "
    "sanitization.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sanitize-undefined-trap-on-error"), maplecl::kHide);

maplecl::Option<bool> oFschedCriticalPathHeuristic({"-fsched-critical-path-heuristic"},
    "  -fsched-critical-path-heuristic  \tEnable the critical path heuristic in the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-critical-path-heuristic"), maplecl::kHide);

maplecl::Option<bool> oFschedDepCountHeuristic({"-fsched-dep-count-heuristic"},
    "  -fsched-dep-count-heuristic \tEnable the dependent count heuristic in the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-dep-count-heuristic"), maplecl::kHide);

maplecl::Option<bool> oFschedGroupHeuristic({"-fsched-group-heuristic"},
    "  -fsched-group-heuristic     \tEnable the group heuristic in the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-group-heuristic"), maplecl::kHide);

maplecl::Option<bool> oFschedLastInsnHeuristic({"-fsched-last-insn-heuristic"},
    "  -fsched-last-insn-heuristic \tEnable the last instruction heuristic in the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-last-insn-heuristic"), maplecl::kHide);

maplecl::Option<bool> oFschedPressure({"-fsched-pressure"},
    "  -fsched-pressure            \tEnable register pressure sensitive insn scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-pressure"), maplecl::kHide);

maplecl::Option<bool> oFschedRankHeuristic({"-fsched-rank-heuristic"},
    "  -fsched-rank-heuristic      \tEnable the rank heuristic in the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-rank-heuristic"), maplecl::kHide);

maplecl::Option<bool> oFschedSpecInsnHeuristic({"-fsched-spec-insn-heuristic"},
    "  -fsched-spec-insn-heuristic \tEnable the speculative instruction heuristic in the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-spec-insn-heuristic"), maplecl::kHide);

maplecl::Option<bool> oFschedSpecLoad({"-fsched-spec-load"},
    "  -fsched-spec-load           \tAllow speculative motion of some loads.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-spec-load"), maplecl::kHide);

maplecl::Option<bool> oFschedSpecLoadDangerous({"-fsched-spec-load-dangerous"},
    "  -fsched-spec-load-dangerous \tAllow speculative motion of more loads.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-spec-load-dangerous"), maplecl::kHide);

maplecl::Option<bool> oFschedStalledInsns({"-fsched-stalled-insns"},
    "  -fsched-stalled-insns       \tAllow premature scheduling of queued insns.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-stalled-insns"), maplecl::kHide);

maplecl::Option<bool> oFschedStalledInsnsDep({"-fsched-stalled-insns-dep"},
    "  -fsched-stalled-insns-dep   \tSet dependence distance checking in premature scheduling of queued insns.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-stalled-insns-dep"), maplecl::kHide);

maplecl::Option<bool> oFschedVerbose({"-fsched-verbose"},
    "  -fsched-verbose             \tSet the verbosity level of the scheduler.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched-verbose"), maplecl::kHide);

maplecl::Option<bool> oFsched2UseSuperblocks({"-fsched2-use-superblocks"},
    "  -fsched2-use-superblocks    \tIf scheduling post reload\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sched2-use-superblocks"), maplecl::kHide);

maplecl::Option<bool> oFscheduleFusion({"-fschedule-fusion"},
    "  -fschedule-fusion           \tPerform a target dependent instruction fusion optimization pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-schedule-fusion"), maplecl::kHide);

maplecl::Option<bool> oFscheduleInsns({"-fschedule-insns"},
    "  -fschedule-insns            \tReschedule instructions before register allocation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-schedule-insns"), maplecl::kHide);

maplecl::Option<bool> oFscheduleInsns2({"-fschedule-insns2"},
    "  -fschedule-insns2           \tReschedule instructions after register allocation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-schedule-insns2"), maplecl::kHide);

maplecl::Option<bool> oFsectionAnchors({"-fsection-anchors"},
    "  -fsection-anchors           \tAccess data in the same section from shared anchor points.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fsection-anchors"), maplecl::kHide);

maplecl::Option<bool> oFselSchedPipelining({"-fsel-sched-pipelining"},
    "  -fsel-sched-pipelining      \tPerform software pipelining of inner loops during selective scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sel-sched-pipelining"), maplecl::kHide);

maplecl::Option<bool> oFselSchedPipeliningOuterLoops({"-fsel-sched-pipelining-outer-loops"},
    "  -fsel-sched-pipelining-outer-loops  \tPerform software pipelining of outer loops during selective scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sel-sched-pipelining-outer-loops"), maplecl::kHide);

maplecl::Option<bool> oFselectiveScheduling({"-fselective-scheduling"},
    "  -fselective-scheduling      \tSchedule instructions using selective scheduling algorithm.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-selective-scheduling"), maplecl::kHide);

maplecl::Option<bool> oFselectiveScheduling2({"-fselective-scheduling2"},
    "  -fselective-scheduling2     \tRun selective scheduling after reload.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-selective-scheduling2"), maplecl::kHide);

maplecl::Option<bool> oFshortEnums({"-fshort-enums"},
    "  -fshort-enums               \tUse the narrowest integer type possible for enumeration types.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-short-enums"), maplecl::kHide);

maplecl::Option<bool> oFshortWchar({"-fshort-wchar"},
    "  -fshort-wchar               \tForce the underlying type for 'wchar_t' to be 'unsigned short'.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-short-wchar"), maplecl::kHide);

maplecl::Option<bool> oFshrinkWrap({"-fshrink-wrap"},
    "  -fshrink-wrap               \tEmit function prologues only before parts of the function that need it\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-shrink-wrap"), maplecl::kHide);

maplecl::Option<bool> oFshrinkWrapSeparate({"-fshrink-wrap-separate"},
    "  -fshrink-wrap-separate      \tShrink-wrap parts of the prologue and epilogue separately.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-shrink-wrap-separate"), maplecl::kHide);

maplecl::Option<bool> oFsignalingNans({"-fsignaling-nans"},
    "  -fsignaling-nans            \tDisable optimizations observable by IEEE signaling NaNs.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-signaling-nans"), maplecl::kHide);

maplecl::Option<bool> oFsignedBitfields({"-fsigned-bitfields"},
    "  -fsigned-bitfields          \tWhen 'signed' or 'unsigned' is not given make the bitfield signed.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-signed-bitfields"), maplecl::kHide);

maplecl::Option<bool> oFsimdCostModel({"-fsimd-cost-model"},
    "  -fsimd-cost-model           \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-simd-cost-model"), maplecl::kHide);

maplecl::Option<bool> oFsinglePrecisionConstant({"-fsingle-precision-constant"},
    "  -fsingle-precision-constant \tConvert floating point constants to single precision constants.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-single-precision-constant"), maplecl::kHide);

maplecl::Option<bool> oFsizedDeallocation({"-fsized-deallocation"},
    "  -fsized-deallocation        \tEnable C++14 sized deallocation support.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sized-deallocation"), maplecl::kHide);

maplecl::Option<bool> oFsplitIvsInUnroller({"-fsplit-ivs-in-unroller"},
    "  -fsplit-ivs-in-unroller     \tSplit lifetimes of induction variables when loops are unrolled.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-split-ivs-in-unroller"), maplecl::kHide);

maplecl::Option<bool> oFsplitLoops({"-fsplit-loops"},
    "  -fsplit-loops               \tPerform loop splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-split-loops"), maplecl::kHide);

maplecl::Option<bool> oFsplitPaths({"-fsplit-paths"},
    "  -fsplit-paths               \tSplit paths leading to loop backedges.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-split-paths"), maplecl::kHide);

maplecl::Option<bool> oFsplitStack({"-fsplit-stack"},
    "  -fsplit-stack               \tGenerate discontiguous stack frames.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-split-stack"), maplecl::kHide);

maplecl::Option<bool> oFsplitWideTypes({"-fsplit-wide-types"},
    "  -fsplit-wide-types          \tSplit wide types into independent registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-split-wide-types"), maplecl::kHide);

maplecl::Option<bool> oFssaBackprop({"-fssa-backprop"},
    "  -fssa-backprop              \tEnable backward propagation of use properties at the SSA level.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ssa-backprop"), maplecl::kHide);

maplecl::Option<bool> oFssaPhiopt({"-fssa-phiopt"},
    "  -fssa-phiopt                \tOptimize conditional patterns using SSA PHI nodes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ssa-phiopt"), maplecl::kHide);

maplecl::Option<bool> oFssoStruct({"-fsso-struct"},
    "  -fsso-struct                \tSet the default scalar storage order.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sso-struct"), maplecl::kHide);

maplecl::Option<bool> oFstackCheck({"-fstack-check"},
    "  -fstack-check               \tInsert stack checking code into the program.  Same as -fstack-check=specific.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stack-check"), maplecl::kHide);

maplecl::Option<bool> oFstackProtector({"-fstack-protector"},
    "  -fstack-protector           \tUse propolice as a stack protection method.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stack-protector"), maplecl::kHide);

maplecl::Option<bool> oFstackProtectorExplicit({"-fstack-protector-explicit"},
    "  -fstack-protector-explicit  \tUse stack protection method only for functions with the stack_protect "
    "attribute.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stack-protector-explicit"), maplecl::kHide);

maplecl::Option<bool> oFstackUsage({"-fstack-usage"},
    "  -fstack-usage               \tOutput stack usage information on a per-function basis.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stack-usage"), maplecl::kHide);

maplecl::Option<bool> oFstats({"-fstats"},
    "  -fstats                     \tDisplay statistics accumulated during compilation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stats"), maplecl::kHide);

maplecl::Option<bool> oFstdargOpt({"-fstdarg-opt"},
    "  -fstdarg-opt                \tOptimize amount of stdarg registers saved to stack at start of function.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-stdarg-opt"), maplecl::kHide);

maplecl::Option<bool> oFstoreMerging({"-fstore-merging"},
    "  -fstore-merging             \tMerge adjacent stores.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-store-merging"), maplecl::kHide);

maplecl::Option<bool> oFstrictEnums({"-fstrict-enums"},
    "  -fstrict-enums              \tAssume that values of enumeration type are always within the minimum range "
    "of that type.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-strict-enums"), maplecl::kHide);

maplecl::Option<bool> oFstrictOverflow({"-fstrict-overflow"},
    "  -fstrict-overflow           \tTreat signed overflow as undefined.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-strict-overflow"), maplecl::kHide);

maplecl::Option<bool> oFstrictVolatileBitfields({"-fstrict-volatile-bitfields"},
    "  -fstrict-volatile-bitfields \tForce bitfield accesses to match their type width.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-strict-volatile-bitfields"), maplecl::kHide);

maplecl::Option<bool> oFsyncLibcalls({"-fsync-libcalls"},
    "  -fsync-libcalls             \tImplement __atomic operations via libcalls to legacy __sync functions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sync-libcalls"), maplecl::kHide);

maplecl::Option<bool> oFsyntaxOnly({"-fsyntax-only"},
    "  -fsyntax-only               \tCheck for syntax errors\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-syntax-only"), maplecl::kHide);

maplecl::Option<bool> oFtestCoverage({"-ftest-coverage"},
    "  -ftest-coverage             \tCreate data files needed by \"gcov\".\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-test-coverage"), maplecl::kHide);

maplecl::Option<bool> oFthreadJumps({"-fthread-jumps"},
    "  -fthread-jumps              \tPerform jump threading optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-thread-jumps"), maplecl::kHide);

maplecl::Option<bool> oFtimeReport({"-ftime-report"},
    "  -ftime-report               \tReport the time taken by each compiler pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-time-report"), maplecl::kHide);

maplecl::Option<bool> oFtimeReportDetails({"-ftime-report-details"},
    "  -ftime-report-details       \tRecord times taken by sub-phases separately.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-time-report-details"), maplecl::kHide);

maplecl::Option<bool> oFtracer({"-ftracer"},
    "  -ftracer                    \tPerform superblock formation via tail duplication.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tracer"), maplecl::kHide);

maplecl::Option<bool> oFtrackMacroExpansion({"-ftrack-macro-expansion"},
    "  -ftrack-macro-expansion     \tTrack locations of tokens across macro expansions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-track-macro-expansion"), maplecl::kHide);

maplecl::Option<bool> oFtrampolines({"-ftrampolines"},
    "  -ftrampolines               \tFor targets that normally need trampolines for nested functions\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-trampolines"), maplecl::kHide);

maplecl::Option<bool> oFtrapv({"-ftrapv"},
    "  -ftrapv                     \tTrap for signed overflow in addition\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-trapv"), maplecl::kHide);

maplecl::Option<bool> oFtreeBitCcp({"-ftree-bit-ccp"},
    "  -ftree-bit-ccp              \tEnable SSA-BIT-CCP optimization on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-bit-ccp"), maplecl::kHide);

maplecl::Option<bool> oFtreeBuiltinCallDce({"-ftree-builtin-call-dce"},
    "  -ftree-builtin-call-dce     \tEnable conditional dead code elimination for builtin calls.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-builtin-call-dce"), maplecl::kHide);

maplecl::Option<bool> oFtreeCcp({"-ftree-ccp"},
    "  -ftree-ccp                  \tEnable SSA-CCP optimization on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-ccp"), maplecl::kHide);

maplecl::Option<bool> oFtreeCh({"-ftree-ch"},
    "  -ftree-ch                   \tEnable loop header copying on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-ch"), maplecl::kHide);

maplecl::Option<bool> oFtreeCoalesceVars({"-ftree-coalesce-vars"},
    "  -ftree-coalesce-vars        \tEnable SSA coalescing of user variables.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-coalesce-vars"), maplecl::kHide);

maplecl::Option<bool> oFtreeCopyProp({"-ftree-copy-prop"},
    "  -ftree-copy-prop            \tEnable copy propagation on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-copy-prop"), maplecl::kHide);

maplecl::Option<bool> oFtreeDce({"-ftree-dce"},
    "  -ftree-dce                  \tEnable SSA dead code elimination optimization on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-dce"), maplecl::kHide);

maplecl::Option<bool> oFtreeDominatorOpts({"-ftree-dominator-opts"},
    "  -ftree-dominator-opts       \tEnable dominator optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-dominator-opts"), maplecl::kHide);

maplecl::Option<bool> oFtreeDse({"-ftree-dse"},
    "  -ftree-dse                  \tEnable dead store elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-dse"), maplecl::kHide);

maplecl::Option<bool> oFtreeForwprop({"-ftree-forwprop"},
    "  -ftree-forwprop             \tEnable forward propagation on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-forwprop"), maplecl::kHide);

maplecl::Option<bool> oFtreeFre({"-ftree-fre"},
    "  -ftree-fre                  \tEnable Full Redundancy Elimination (FRE) on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-fre"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopDistributePatterns({"-ftree-loop-distribute-patterns"},
    "  -ftree-loop-distribute-patterns  \tEnable loop distribution for patterns transformed into a library call.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-distribute-patterns"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopDistribution({"-ftree-loop-distribution"},
    "  -ftree-loop-distribution    \tEnable loop distribution on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-distribution"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopIfConvert({"-ftree-loop-if-convert"},
    "  -ftree-loop-if-convert      \tConvert conditional jumps in innermost loops to branchless equivalents.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-if-convert"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopIm({"-ftree-loop-im"},
    "  -ftree-loop-im              \tEnable loop invariant motion on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-im"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopIvcanon({"-ftree-loop-ivcanon"},
    "  -ftree-loop-ivcanon         \tCreate canonical induction variables in loops.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-ivcanon"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopLinear({"-ftree-loop-linear"},
    "  -ftree-loop-linear          \tEnable loop nest transforms.  Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-linear"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopOptimize({"-ftree-loop-optimize"},
    "  -ftree-loop-optimize        \tEnable loop optimizations on tree level.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-optimize"), maplecl::kHide);

maplecl::Option<bool> oFtreeLoopVectorize({"-ftree-loop-vectorize"},
    "  -ftree-loop-vectorize       \tEnable loop vectorization on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-loop-vectorize"), maplecl::kHide);

maplecl::Option<bool> oFtreeParallelizeLoops({"-ftree-parallelize-loops"},
    "  -ftree-parallelize-loops    \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-parallelize-loops"), maplecl::kHide);

maplecl::Option<bool> oFtreePartialPre({"-ftree-partial-pre"},
    "  -ftree-partial-pre          \tIn SSA-PRE optimization on trees\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-partial-pre"), maplecl::kHide);

maplecl::Option<bool> oFtreePhiprop({"-ftree-phiprop"},
    "  -ftree-phiprop              \tEnable hoisting loads from conditional pointers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-phiprop"), maplecl::kHide);

maplecl::Option<bool> oFtreePre({"-ftree-pre"},
    "  -ftree-pre                  \tEnable SSA-PRE optimization on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-pre"), maplecl::kHide);

maplecl::Option<bool> oFtreePta({"-ftree-pta"},
    "  -ftree-pta                  \tPerform function-local points-to analysis on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-pta"), maplecl::kHide);

maplecl::Option<bool> oFtreeReassoc({"-ftree-reassoc"},
    "  -ftree-reassoc              \tEnable reassociation on tree level.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-reassoc"), maplecl::kHide);

maplecl::Option<bool> oFtreeSink({"-ftree-sink"},
    "  -ftree-sink                 \tEnable SSA code sinking on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-sink"), maplecl::kHide);

maplecl::Option<bool> oFtreeSlpVectorize({"-ftree-slp-vectorize"},
    "  -ftree-slp-vectorize        \tEnable basic block vectorization (SLP) on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-slp-vectorize"), maplecl::kHide);

maplecl::Option<bool> oFtreeSlsr({"-ftree-slsr"},
    "  -ftree-slsr                 \tPerform straight-line strength reduction.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-slsr"), maplecl::kHide);

maplecl::Option<bool> oFtreeSra({"-ftree-sra"},
    "  -ftree-sra                  \tPerform scalar replacement of aggregates.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-sra"), maplecl::kHide);

maplecl::Option<bool> oFtreeSwitchConversion({"-ftree-switch-conversion"},
    "  -ftree-switch-conversion    \tPerform conversions of switch initializations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-switch-conversion"), maplecl::kHide);

maplecl::Option<bool> oFtreeTailMerge({"-ftree-tail-merge"},
    "  -ftree-tail-merge           \tEnable tail merging on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-tail-merge"), maplecl::kHide);

maplecl::Option<bool> oFtreeTer({"-ftree-ter"},
    "  -ftree-ter                  \tReplace temporary expressions in the SSA->normal pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-ter"), maplecl::kHide);

maplecl::Option<bool> oFtreeVrp({"-ftree-vrp"},
    "  -ftree-vrp                  \tPerform Value Range Propagation on trees.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tree-vrp"), maplecl::kHide);

maplecl::Option<bool> oFunconstrainedCommons({"-funconstrained-commons"},
    "  -funconstrained-commons     \tAssume common declarations may be overridden with ones with a larger "
    "trailing array.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unconstrained-commons"), maplecl::kHide);

maplecl::Option<bool> oFunitAtATime({"-funit-at-a-time"},
    "  -funit-at-a-time            \tCompile whole compilation unit at a time.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unit-at-a-time"), maplecl::kHide);

maplecl::Option<bool> oFunrollAllLoops({"-funroll-all-loops"},
    "  -funroll-all-loops          \tPerform loop unrolling for all loops.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unroll-all-loops"), maplecl::kHide);

maplecl::Option<bool> oFunrollLoops({"-funroll-loops"},
    "  -funroll-loops              \tPerform loop unrolling when iteration count is known.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unroll-loops"), maplecl::kHide);

maplecl::Option<bool> oFunsafeMathOptimizations({"-funsafe-math-optimizations"},
    "  -funsafe-math-optimizations \tAllow math optimizations that may violate IEEE or ISO standards.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unsafe-math-optimizations"), maplecl::kHide);

maplecl::Option<bool> oFunsignedBitfields({"-funsigned-bitfields"},
    "  -funsigned-bitfields        \tWhen \"signed\" or \"unsigned\" is not given make the bitfield unsigned.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unsigned-bitfields"), maplecl::kHide);

maplecl::Option<bool> oFunswitchLoops({"-funswitch-loops"},
    "  -funswitch-loops            \tPerform loop unswitching.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unswitch-loops"), maplecl::kHide);

maplecl::Option<bool> oFunwindTables({"-funwind-tables"},
    "  -funwind-tables             \tJust generate unwind tables for exception handling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-unwind-tables"), maplecl::kHide);

maplecl::Option<bool> oFuseCxaAtexit({"-fuse-cxa-atexit"},
    "  -fuse-cxa-atexit            \tUse __cxa_atexit to register destructors.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-use-cxa-atexit"), maplecl::kHide);

maplecl::Option<bool> oFuseLinkerPlugin({"-fuse-linker-plugin"},
    "  -fuse-linker-plugin         \tEnables the use of a linker plugin during link-time optimization.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-use-linker-plugin"), maplecl::kHide);

maplecl::Option<bool> oFvarTracking({"-fvar-tracking"},
    "  -fvar-tracking              \tPerform variable tracking.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-var-tracking"), maplecl::kHide);

maplecl::Option<bool> oFvarTrackingAssignments({"-fvar-tracking-assignments"},
    "  -fvar-tracking-assignments  \tPerform variable tracking by annotating assignments.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-var-tracking-assignments"), maplecl::kHide);

maplecl::Option<bool> oFvarTrackingAssignmentsToggle({"-fvar-tracking-assignments-toggle"},
    "  -fvar-tracking-assignments-toggle  \tToggle -fvar-tracking-assignments.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-var-tracking-assignments-toggle"), maplecl::kHide);

maplecl::Option<bool> oFvariableExpansionInUnroller({"-fvariable-expansion-in-unroller"},
    "  -fvariable-expansion-in-unroller  \tApply variable expansion when loops are unrolled.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-variable-expansion-in-unroller"), maplecl::kHide);

maplecl::Option<bool> oFvectCostModel({"-fvect-cost-model"},
    "  -fvect-cost-model           \tEnables the dynamic vectorizer cost model. Preserved for backward "
    "compatibility.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-vect-cost-model"), maplecl::kHide);

maplecl::Option<bool> oFverboseAsm({"-fverbose-asm"},
    "  -fverbose-asm                  \tAdd extra commentary to assembler output.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-verbose-asm"), maplecl::kHide);

maplecl::Option<bool> oFvisibilityInlinesHidden({"-fvisibility-inlines-hidden"},
    "  -fvisibility-inlines-hidden \tMarks all inlined functions and methods as having hidden visibility.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-visibility-inlines-hidden"), maplecl::kHide);

maplecl::Option<bool> oFvisibilityMsCompat({"-fvisibility-ms-compat"},
    "  -fvisibility-ms-compat      \tChanges visibility to match Microsoft Visual Studio by default.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-visibility-ms-compat"), maplecl::kHide);

maplecl::Option<bool> oFvpt({"-fvpt"},
    "  -fvpt                       \tUse expression value profiles in optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-vpt"), maplecl::kHide);

maplecl::Option<bool> oFvtableVerify({"-fvtable-verify"},
    "  -fvtable-verify             \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-vtable-verify"), maplecl::kHide);

maplecl::Option<bool> oFvtvCounts({"-fvtv-counts"},
    "  -fvtv-counts                \tOutput vtable verification counters.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-vtv-counts"), maplecl::kHide);

maplecl::Option<bool> oFvtvDebug({"-fvtv-debug"},
    "  -fvtv-debug                 \tOutput vtable verification pointer sets information.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-vtv-debug"), maplecl::kHide);

maplecl::Option<bool> oFweb({"-fweb"},
    "  -fweb                       \tConstruct webs and split unrelated uses of single variable.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-web"), maplecl::kHide);

maplecl::Option<bool> oFwholeProgram({"-fwhole-program"},
    "  -fwhole-program             \tPerform whole program optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-whole-program"), maplecl::kHide);

maplecl::Option<bool> oMwarnDynamicstack({"-mwarn-dynamicstack"},
    "  -mwarn-dynamicstack         \tEmit a warning if the function calls alloca or uses dynamically-sized arrays.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMwarnMcu({"-mwarn-mcu"},
    "  -mwarn-mcu                  \tThis option enables or disables warnings about conflicts between the MCU name "
    "specified by the -mmcu option and the ISA set by the -mcpu option and/or the hardware multiply support set by "
    "the -mhwmult option.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-warn-mcu"), maplecl::kHide);

maplecl::Option<bool> oMwarnMultipleFastInterrupts({"-mwarn-multiple-fast-interrupts"},
    "  -mwarn-multiple-fast-interrupts  \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-warn-multiple-fast-interrupts"), maplecl::kHide);

maplecl::Option<bool> oMwarnReloc({"-mwarn-reloc"},
    "  -mwarn-reloc                \t-mwarn-reloc generates a warning instead.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-merror-reloc"), maplecl::kHide);

maplecl::Option<bool> oMwideBitfields({"-mwide-bitfields"},
    "  -mwide-bitfields            \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-wide-bitfields"), maplecl::kHide);

maplecl::Option<bool> oMwin32({"-mwin32"},
    "  -mwin32                     \tThis option is available for Cygwin and MinGW targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMwindows({"-mwindows"},
    "  -mwindows                   \tThis option is available for MinGW targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMwordRelocations({"-mword-relocations"},
    "  -mword-relocations          \tOnly generate absolute relocations on word-sized values (i.e. R_ARM_ABS32).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMx32({"-mx32"},
    "  -mx32                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxgot({"-mxgot"},
    "  -mxgot                      \tLift (do not lift) the usual restrictions on the size of the global offset "
    "table.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-xgot"), maplecl::kHide);

maplecl::Option<bool> oMxilinxFpu({"-mxilinx-fpu"},
    "  -mxilinx-fpu                \tPerform optimizations for the floating-point unit on Xilinx PPC 405/440.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlBarrelShift({"-mxl-barrel-shift"},
    "  -mxl-barrel-shift           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlCompat({"-mxl-compat"},
    "  -mxl-compat                 \tProduce code that conforms more closely to IBM XL compiler semantics when "
    "using AIX-compatible ABI. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-xl-compat"), maplecl::kHide);

maplecl::Option<bool> oMxlFloatConvert({"-mxl-float-convert"},
    "  -mxl-float-convert          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlFloatSqrt({"-mxl-float-sqrt"},
    "  -mxl-float-sqrt             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlGpOpt({"-mxl-gp-opt"},
    "  -mxl-gp-opt                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlMultiplyHigh({"-mxl-multiply-high"},
    "  -mxl-multiply-high          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlPatternCompare({"-mxl-pattern-compare"},
    "  -mxl-pattern-compare       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlReorder({"-mxl-reorder"},
    "  -mxl-reorder                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlSoftDiv({"-mxl-soft-div"},
    "  -mxl-soft-div               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlSoftMul({"-mxl-soft-mul"},
    "  -mxl-soft-mul               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxlStackCheck({"-mxl-stack-check"},
    "  -mxl-stack-check            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxop({"-mxop"},
    "  -mxop                       \tThese switches enable the use of instructions in the mxop.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxpa({"-mxpa"},
    "  -mxpa                       \tUse the MIPS eXtended Physical Address (XPA) instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-xpa"), maplecl::kHide);

maplecl::Option<bool> oMxsave({"-mxsave"},
    "  -mxsave                     \tThese switches enable the use of instructions in the mxsave.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxsavec({"-mxsavec"},
    "  -mxsavec                    \tThese switches enable the use of instructions in the mxsavec.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxsaveopt({"-mxsaveopt"},
    "  -mxsaveopt                  \tThese switches enable the use of instructions in the mxsaveopt.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxsaves({"-mxsaves"},
    "  -mxsaves                    \tThese switches enable the use of instructions in the mxsaves.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMxy({"-mxy"},
    "  -mxy                        \tPassed down to the assembler to enable the XY memory extension. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMyellowknife({"-myellowknife"},
    "  -myellowknife               \tOn embedded PowerPC systems, assume that the startup module is called crt0.o "
    "and the standard C libraries are libyk.a and libc.a.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMzarch({"-mzarch"},
    "  -mzarch                     \tWhen -mzarch is specified, generate code using the instructions available on "
    "z/Architecture.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);


maplecl::Option<bool> oMzdcbranch({"-mzdcbranch"},
    "  -mzdcbranch                 \tAssume (do not assume) that zero displacement conditional branch instructions "
    "bt and bf are fast. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-zdcbranch"), maplecl::kHide);

maplecl::Option<bool> oMzeroExtend({"-mzero-extend"},
    "  -mzero-extend               \tWhen reading data from memory in sizes shorter than 64 bits, use zero-extending"
    " load instructions by default, rather than sign-extending ones.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-zero-extend"), maplecl::kHide);

maplecl::Option<bool> oMzvector({"-mzvector"},
    "  -mzvector                   \tThe -mzvector option enables vector language extensions and builtins using "
    "instructions available with the vector extension facility introduced with the IBM z13 machine generation. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-zvector"), maplecl::kHide);

maplecl::Option<bool> oNo80387({"-no-80387"},
    "  -no-80387                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNoCanonicalPrefixes({"-no-canonical-prefixes"},
    "  -no-canonical-prefixes      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNoIntegratedCpp({"-no-integrated-cpp"},
    "  -no-integrated-cpp          \tPerform preprocessing as a separate pass before compilation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNoSysrootSuffix({"-no-sysroot-suffix"},
    "  -no-sysroot-suffix          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNoall_load({"-noall_load"},
    "  -noall_load                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNocpp({"-nocpp"},
    "  -nocpp                      \tDisable preprocessing.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNodevicelib({"-nodevicelib"},
    "  -nodevicelib                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNofixprebinding({"-nofixprebinding"},
    "  -nofixprebinding            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNolibdld({"-nolibdld"},
    "  -nolibdld                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNomultidefs({"-nomultidefs"},
    "  -nomultidefs                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNonStatic({"-non-static"},
    "  -non-static                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNoprebind({"-noprebind"},
    "  -noprebind                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNoseglinkedit({"-noseglinkedit"},
    "  -noseglinkedit              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNostdinc({"-nostdinc++"},
    "  -nostdinc++                 \tDo not search standard system include directories for C++.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oNo_dead_strip_inits_and_terms({"-no_dead_strip_inits_and_terms"},
    "  -no_dead_strip_inits_and_terms             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oOfast({"-Ofast"},
    "  -Ofast                      \tOptimize for speed disregarding exact standards compliance.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oOg({"-Og"},
    "  -Og                         \tOptimize for debugging experience rather than speed or size.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oP({"-p"},
    "  -p                          \tEnable function profiling.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPagezero_size({"-pagezero_size"},
    "  -pagezero_size              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oParam({"--param"},
    "  --param <param>=<value>     \tSet parameter <param> to value.  See below for a complete list of parameters.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPassExitCodes({"-pass-exit-codes"},
    "  -pass-exit-codes            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPedantic({"-pedantic"},
    "  -pedantic                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPedanticErrors({"-pedantic-errors"},
    "  -pedantic-errors            \tLike -pedantic but issue them as errors.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPg({"-pg"},
    "  -pg                         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPlt({"-plt"},
    "  -plt                        \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrebind({"-prebind"},
    "  -prebind                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrebind_all_twolevel_modules({"-prebind_all_twolevel_modules"},
    "  -prebind_all_twolevel_modules  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintFileName({"-print-file-name"},
    "  -print-file-name            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintLibgccFileName({"-print-libgcc-file-name"},
    "  -print-libgcc-file-name     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintMultiDirectory({"-print-multi-directory"},
    "  -print-multi-directory      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintMultiLib({"-print-multi-lib"},
    "  -print-multi-lib            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintMultiOsDirectory({"-print-multi-os-directory"},
    "  -print-multi-os-directory   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintMultiarch({"-print-multiarch"},
    "  -print-multiarch            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintObjcRuntimeInfo({"-print-objc-runtime-info"},
    "  -print-objc-runtime-info    \tGenerate C header of platform-specific features.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintProgName({"-print-prog-name"},
    "  -print-prog-name            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintSearchDirs({"-print-search-dirs"},
    "  -print-search-dirs          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintSysroot({"-print-sysroot"},
    "  -print-sysroot              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrintSysrootHeadersSuffix({"-print-sysroot-headers-suffix"},
    "  -print-sysroot-headers-suffix  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPrivate_bundle({"-private_bundle"},
    "  -private_bundle             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oPthreads({"-pthreads"},
    "  -pthreads                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oQ({"-Q"},
    "  -Q                          \tMakes the compiler print out each function name as it is compiled, and print "
    "some statistics about each pass when it finishes.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oQn({"-Qn"},
    "  -Qn                         \tRefrain from adding .ident directives to the output file (this is the default).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oQy({"-Qy"},
    "  -Qy                         \tIdentify the versions of each tool used by the compiler, in a .ident assembler "
    "directive in the output.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oRead_only_relocs({"-read_only_relocs"},
    "  -read_only_relocs           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oRemap({"-remap"},
    "  -remap                      \tRemap file names when including files.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSectalign({"-sectalign"},
    "  -sectalign                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSectcreate({"-sectcreate"},
    "  -sectcreate                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSectobjectsymbols({"-sectobjectsymbols"},
    "  -sectobjectsymbols          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSectorder({"-sectorder"},
    "  -sectorder                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSeg1addr({"-seg1addr"},
    "  -seg1addr                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSegaddr({"-segaddr"},
    "  -segaddr                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSeglinkedit({"-seglinkedit"},
    "  -seglinkedit                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSegprot({"-segprot"},
    "  -segprot                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSegs_read_only_addr({"-segs_read_only_addr"},
    "  -segs_read_only_addr        \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSegs_read_write_addr({"-segs_read_write_addr"},
    "  -segs_read_write_addr       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSeg_addr_table({"-seg_addr_table"},
    "  -seg_addr_table             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSeg_addr_table_filename({"-seg_addr_table_filename"},
    "  -seg_addr_table_filename    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSim({"-sim"},
    "  -sim                        \tThis option, recognized for the cris-axis-elf, arranges to link with "
    "input-output functions from a simulator library\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSim2({"-sim2"},
    "  -sim2                       \tLike -sim, but pass linker options to locate initialized data at "
    "s0x40000000 and zero-initialized data at 0x80000000.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSingle_module({"-single_module"},
    "  -single_module              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSub_library({"-sub_library"},
    "  -sub_library                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSub_umbrella({"-sub_umbrella"},
    "  -sub_umbrella               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oTargetHelp({"-target-help"},
    "  -target-help                \tPrint (on the standard output) a description of target-specific command-line "
    "options for each tool.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oThreads({"-threads"},
    "  -threads                    \tAdd support for multithreading with the dce thread library under HP-UX. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oTnoAndroidCc({"-tno-android-cc"},
    "  -tno-android-cc             \tDisable compilation effects of -mandroid, i.e., do not enable -mbionic, -fPIC,"
    " -fno-exceptions and -fno-rtti by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oTnoAndroidLd({"-tno-android-ld"},
    "  -tno-android-ld             \tDisable linking effects of -mandroid, i.e., pass standard Linux linking options"
    " to the linker.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oTraditional({"-traditional"},
    "  -traditional                \tEnable traditional preprocessing. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oTraditionalCpp({"-traditional-cpp"},
    "  -traditional-cpp            \tEnable traditional preprocessing.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oTwolevel_namespace({"-twolevel_namespace"},
    "  -twolevel_namespace         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oUmbrella({"-umbrella"},
    "  -umbrella                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oUndefined({"-undefined"},
    "  -undefined                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oUnexported_symbols_list({"-unexported_symbols_list"},
    "  -unexported_symbols_list    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

}