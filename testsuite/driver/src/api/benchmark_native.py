import os
import re
from api.shell_operator import ShellOperator


class BenchmarkNative(ShellOperator):

    def __init__(self, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.command = ""
        self.native_lib_name = ""
        self.native_src = ""
        self.native_include = "-I${MAPLE_ROOT}/../libnativehelper/include_jni"
        self.native_linker = ""

    def get_command(self, variables):
        if "NATIVE_LIB_NAME" in variables:
            self.native_lib_name = variables["NATIVE_LIB_NAME"]
        if "NATIVE_SRC" in variables:
            srcs = variables["NATIVE_SRC"].split(":")
            for src in srcs:
                self.native_src += " " + src
        if "NATIVE_INCLUDE" in variables:
            includes = variables["NATIVE_INCLUDE"].split(":")
            for include in includes:
                if include != "":
                    self.native_include += " -I${MAPLE_ROOT}/../" + include
        if "NATIVE_LINKE" in variables:
            links = variables["NATIVE_LINKE"].split(":")
            for link in links:
                if link != "":
                    self.native_linker += " ${MAPLE_ROOT}/../" + link
        self.command = "${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/bin/clang++ " + self.native_include + " " + self.native_src + " ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libc/crtbegin_so/android_arm64_armv8-a_core/crtbegin_so.o ${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/lib64/clang/9.0.3/lib/linux/libclang_rt.builtins-aarch64-android.a ${MAPLE_ROOT}/../prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/aarch64-linux-android/lib64/libatomic.a ${MAPLE_ROOT}/../out/soong/.intermediates/build/soong/libgcc_stripped/android_arm64_armv8-a_core_static/libgcc_stripped.a " + self.native_linker + " ${MAPLE_ROOT}/../out/soong/.intermediates/external/libcxx/libc++/android_arm64_armv8-a_core_shared/libc++.so ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libc/libc/android_arm64_armv8-a_core_shared_10000/libc.so ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libm/libm/android_arm64_armv8-a_core_shared_10000/libm.so ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libdl/libdl/android_arm64_armv8-a_core_shared_10000/libdl.so ${MAPLE_ROOT}/../out/soong/.intermediates/bionic/libc/crtend_so/android_arm64_armv8-a_core/obj/bionic/libc/arch-common/bionic/crtend_so.o -o ${BENCHMARK_ACTION}/lib" + self.native_lib_name + ".so -nostdlib -Wl,--gc-sections -shared -Wl,-soname,lib" + self.native_lib_name + ".so -target aarch64-linux-android -B${MAPLE_ROOT}/../prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/aarch64-linux-android/bin -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--build-id=md5 -Wl,--warn-shared-textrel -Wl,--fatal-warnings -Wl,--no-undefined-version -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libgcc_stripped.a -fuse-ld=lld -Wl,--pack-dyn-relocs=android+relr -Wl,--use-android-relr-tags -Wl,--no-undefined -Wl,--hash-style=gnu -Wl,--icf=safe -Wl,-z,max-page-size=4096 ${MAPLE_ROOT}/../prebuilts/clang/host/linux-x86/clang-r353983c/lib64/clang/9.0.3/lib/linux/libclang_rt.ubsan_minimal-aarch64-android.a -Wl,--exclude-libs,libclang_rt.ubsan_minimal-aarch64-android.a -Wl,-execute-only -fPIC"
        return super().get_final_command(variables)