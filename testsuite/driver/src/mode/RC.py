#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

from api import *
from env_var import EnvVar

RC = {
    "compile": [
        BenchmarkVogar(),
        Shell(
            "mv ${BENCHMARK_ACTION}/${BENCHMARK_ACTION}.dex.jar ${BENCHMARK_ACTION}/${BENCHMARK_ACTION}.jar;"
            "if [ -d \"${BENCHMARK_ACTION}/dex\" ]; then"
            "    rm -rf ${BENCHMARK_ACTION}/dex;"
            "fi;"
            "unzip -q ${BENCHMARK_ACTION}/${BENCHMARK_ACTION}.jar -d ${BENCHMARK_ACTION}/dex"
        ),
        Maple(
            maple="${MAPLE_ROOT}/../out/soong/host/linux-x86/bin/maple",
            run=["dex2mpl"],
            option={
                "dex2mpl": "-checktool -check-invoke -invoke-checklist=${MAPLE_ROOT}/mrt/codetricks/profile.pv/classloaderInvocation.list -check-incomplete -incomplete-whitelist=${MAPLE_ROOT}/mrt/codetricks/compile/incomplete.list -incomplete-detail -staticstringcheck --inlinefunclist=${MAPLE_ROOT}/mrt/codetricks/profile.pv/to_inline.list -dexcatch -litprofile=${MAPLE_ROOT}/mrt/codetricks/profile.pv/meta.list -output=${BENCHMARK_ACTION}/dex/ -mplt=${MAPLE_ROOT}/../out/soong/.intermediates/vendor/huawei/maple/Lib/core/libmaplecore-all/android_arm64_armv8-a_core_shared/obj/classes.mplt"
            },
            global_option="",
            infile="${BENCHMARK_ACTION}/dex/classes.dex"
        ),
        Maple(
            maple="${MAPLE_ROOT}/../out/soong/host/linux-x86/bin/maple",
            run=["mplipa"],
            option={
                "mplipa": "--effectipa --quiet --inlinefunclist=${MAPLE_ROOT}/mrt/codetricks/profile.pv/inline_funcs.list"
            },
            global_option="",
            infile="${BENCHMARK_ACTION}/dex/classes.mpl > /dev/null"
        ),
        Maple(
            maple="${MAPLE_ROOT}/../out/soong/host/linux-x86/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "--inlinefunclist=${MAPLE_ROOT}/mrt/codetricks/profile.pv/inline_funcs.list -O2 --quiet --no-ignoreipa",
                "mpl2mpl": "-regnativefunc --quiet -O2 --usewhiteclass --maplelinker --dump-muid --check_cl_invocation=${MAPLE_ROOT}/mrt/codetricks/profile.pv/classloaderInvocation.list --regnative-dynamic-only",
                "mplcg": "-O2 --quiet --no-pie --nativeopt --verbose-asm --gen-c-macro-def --maplelinker --gsrc --duplicate_asm_list2=${MAPLE_ROOT}/mrt/compiler-rt/src/arch/arm64/fastFuncs.S  --fPIC"
            },
            global_option="--genVtableImpl",
            infile="${BENCHMARK_ACTION}/dex/classes.mpl"
        ),
        Shell(
            "${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/bin/clang -target aarch64-linux-android -g -c -x assembler-with-cpp -D__ASSEMBLY__ -DUSE_32BIT_REF -MD -MF ${BENCHMARK_ACTION}/dex/classes.d -o ${BENCHMARK_ACTION}/dex/classes.o ${BENCHMARK_ACTION}/dex/classes.VtableImpl.s"
        ),
        Shell(
            "${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/bin/llvm-objcopy --rename-section .debug_info=.maple_java_debug_info --rename-section .debug_abbrev=.maple_java_debug_abbrev --rename-section .debug_line=.maple_java_debug_line --rename-section .debug_aranges=.maple_java_debug_aranges --rename-section .debug_ranges=.maple_java_debug_ranges  ${BENCHMARK_ACTION}/dex/classes.o"
        ),
        Shell(
            "${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/bin/clang++ -nostdlib -Wl,-soname,libmaple${BENCHMARK_ACTION}.so -Wl,--gc-sections -shared ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libc/crtbegin_so/android_arm64_armv8-a_core/crtbegin_so.o ${BENCHMARK_ACTION}/dex/classes.o -Wl,--whole-archive ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/STATIC_LIBRARIES/mrt_module_init_intermediates/mrt_module_init.a -Wl,--no-whole-archive ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/STATIC_LIBRARIES/libclang_rt.ubsan_minimal-aarch64-android_intermediates/libclang_rt.ubsan_minimal-aarch64-android.a ${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/lib64/clang/9.0.3/lib/linux//libclang_rt.builtins-aarch64-android.a ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/STATIC_LIBRARIES/libatomic_intermediates/libatomic.a ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/STATIC_LIBRARIES/libgcc_intermediates/libgcc.a -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--build-id=md5 -Wl,--warn-shared-textrel -Wl,--fatal-warnings -Wl,--no-undefined-version -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libgcc_stripped.a -fuse-ld=lld -Wl,--hash-style=gnu -Wl,--icf=safe -Wl,-z,max-page-size=4096 -target aarch64-linux-android -B${MAPLE_ROOT}/../prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/aarch64-linux-android/bin -Wl,-T,${MAPLE_ROOT}/mrt/maplert/linker/maplelld.so.lds -Wl,-execute-only -Wl,--exclude-libs,libclang_rt.ubsan_minimal-aarch64-android.a -Wl,--no-undefined ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libmaplecore-all_intermediates/libmaplecore-all.so ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libmrt_intermediates/libmrt.so ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libcommon_bridge_intermediates/libcommon_bridge.so  ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libc++_intermediates/libc++.so  ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libc_intermediates/libc.so  ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libm_intermediates/libm.so  ${MAPLE_ROOT}/../out/target/product/generic_a15/obj/SHARED_LIBRARIES/libdl_intermediates/libdl.so -o ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}Symbol.so ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libc/crtend_so/android_arm64_armv8-a_core/obj/bionic/libc/arch-common/bionic/crtend_so.o"
        ),
        Shell(
            "CLANG_BIN=${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/bin CROSS_COMPILE=${MAPLE_ROOT}/../prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android- XZ=${MAPLE_ROOT}/../prebuilts/build-tools/linux-x86/bin/xz ${MAPLE_ROOT}/../build/soong/scripts/strip.sh -i ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}Symbol.so -o ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so -d ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so.d  --keep-mini-debug-info"
        ),
        Shell(
            "(${MAPLE_ROOT}/../prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-readelf -d ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so | grep SONAME || echo \"No SONAME for ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so\") > ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so.toc.tmp;"
            "${MAPLE_ROOT}/../prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-readelf --dyn-syms ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so | awk '{$2=\"\"; $3=\"\"; print}' >> ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so.toc.tmp;"
            "mv ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so.toc.tmp ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so.toc;"
            "cp ${BENCHMARK_ACTION}/dex/libmaple${BENCHMARK_ACTION}.so ${BENCHMARK_ACTION}"
        ),
    ],
    "native_compile": [
        BenchmarkNative()
    ]
}
