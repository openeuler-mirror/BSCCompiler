# 环境配置

## 硬件推荐配置

- 2 GHz 双核处理器或者更高等级 CPU

- 2 GB 系统内存及以上

- 200GB 可用磁盘空间

## 开发环境推荐

您需要安装一个 64 位版本的 Ubuntu（Ubuntu 16.04，18.04，20.04 皆可）

```
sudo apt-get -y install clang llvm lld libelf-dev libssl-dev python qemu openjdk-8-jre-headless openjdk-8-jdk-headless cmake
sudo apt-get -y install git build-essential zlib1g-dev libc6-dev-i386 g++-multilib gcc-multilib linux-libc-dev:i386

Ubuntu 16.04:
sudo apt-get -y install gcc-5-aarch64-linux-gnu g++-5-aarch64-linux-gnu

Ubuntu 18.04:
sudo apt-get -y install gcc-7-aarch64-linux-gnu g++-7-aarch64-linux-gnu

Ubuntu 20.04:
sudo apt-get -y install gcc-9-aarch64-linux-gnu g++-9-aarch64-linux-gnu libncurses5
```

## 自动安装工具

```
source build/envsetup.sh arm release
make setup

以下的步骤只是作为参考，需要的工具都已经在 "make setup" 一步自动安装完成。
```

## 安装 Clang 编译器并完成配置（用于编译方舟编译器代码，20.04 已改为使用系统安装的 Clang）

下载**clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04** (具体版本请根据系统版本确定)

LLVM 下载地址：http://releases.llvm.org/download.html#10.0.0

解压并放置到`openarkcompiler/tools`目录

- 修改`openarkcompiler/build/envsetup.sh`文件，将`CLANG_PATH`变量配置为 clang 编译器所在路径，例如：

```
CLANG_PATH = "${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin"
```

其中${MAPLE_ROOT}为 openarkcompiler 源码根目录。

## 安装 Ninja、GN 并完成配置

下载**Ninja(v1.10.0)**及**GN(Linux Version)**

Ninja 下载地址：https://github.com/ninja-build/ninja/releases

GN 下载地址：https://gitee.com/xlnb/gn_binary

将 GN 和 Ninja 可执行程序放置到`openarkcompiler/tools`目录，然后修改这两个文件为可执行:

```
cd openarkcompiler/tools
chmod 775 gn
chmod 775 ninja
```

打开`openarkcompiler/Makefile`文件，将 GN 和 NINJA 两个变量配置为 GN 和 Ninja 可执行程序所在路径。例如：

```
GN := ${MAPLE_ROOT}/tools/gn/gn
NINJA := ${MAPLE_ROOT}/tools/ninja/ninja
```

## 安装 gcc-linaro 并完成配置（用于交叉编译方舟编译器代码）

下载**gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu**

gcc-linaro-7.5.0 下载地址：https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/

解压并放置到`openarkcompiler/tools`目录，并将文件夹更名为`gcc-linaro-7.5.0`。

- 修改`openarkcompiler/build/config.gni`文件，将`GCC_LINARO_PATH`变量配置为 gcc-linaro-7.5.0 所在路径，例如：

```
GCC_LINARO_PATH = "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0"
```

- 修改`openarkcompiler/build/core/maple_variables.mk`文件，将`GCC_LINARO_PATH`配置为 gcc-linaro-7.5.0 所在路径，例如：

```
GCC_LINARO_PATH := ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0
```

## 安装 android-ndk 并完成配置（用于编译方舟编译器代码）

下载**android-ndk-r21b-linux-x86_64.zip**

android-ndk-r21 下载地址：https://developer.android.google.cn/ndk/downloads/

解压并放置到 openarkcompiler/tools 目录，并将文件夹更名为`android-ndk-r21`。

- 修改`openarkcompiler/build/config.gni`文件，将`NDK_PATH`变量配置为 android-ndk-r21 所在路径，例如：

```
NDK_PATH = "${MAPLE_ROOT}/tools/android-ndk-r21"
```

- 修改`openarkcompiler/build/core/maple_variables.mk`文件，将`NDK_PATH`配置为 android-ndk-r21 所在路径，例如：

```
NDK_PATH := ${MAPLE_ROOT}/tools/android-ndk-r21
```

## AOSP 运行环境依赖

当前编译方舟编译器 Sample 应用需要使用到 Java 基础库，我们通过 AOSP 来获取，请使用 Android-10.0.0_r35 版本，暂不支持 Android11 版本。

AOSP 下载地址：https://source.android.com/source/downloading/

下载 AOSP 并编译完成。

- 在 openarkcompiler 目录下新建链接`android/`，并链接到 AOSP 的根目录；
- 将`openarkcompiler/android/out/target/product/generic_arm64/obj/JAVA_LIBRARIES/core-all_intermediates/javalib.jar`拷贝到`openarkcompiler/libjava-core`目录，并命名为`java-core.jar`，同时码云上也提供了编译好的 libcore 的 jar 文件，你可以下载直接使用，下载链接`https://gitee.com/xlnb/aosp_core_bin`；
- 在 openarkcompiler/tools 下新建链接 gcc，并链接到 AOSP 的`openarkcompiler/android/prebuilts/gcc`;
- 在 openarkcompiler/tools 下新建链接 clang-r353983c，并链接到 AOSP 的`openarkcompiler/android/prebuilts/clang/host/linux-x86/clang-r353983c`；
- 修改`openarkcompiler/build/config.gni`和`openarkcompiler/build/core/maple_variables.mk`中`ANDROID_GCC_PATH`和`ANDROID_CLANG_PATH`两个变量，配置为上述 gcc 和 clang-r353982c 的所在路径，例如：

config.gni

```
ANDROID_GCC_PATH = "${MAPLE_ROOT}/tools/gcc"
ANDROID_CLANG_PATH = "${MAPLE_ROOT}/tools/clang-r353983c"
```

maple_variables.mk

```
ANDROID_GCC_PATH := ${MAPLE_ROOT}/tools/gcc
ANDROID_GLANG_PATH := ${MAPLE_ROOT}/tools/clang-r353983c
```

## 构建工具依赖下载

### icu 下载并编译

当前用例编译需要 icu 动态库支持，请使用 icu56.1 版本。

icu 下载地址：http://site.icu-project.org/home

下载 56.1 版本的 icu4c 并编译完成，生成`libicuuc.so`和`libicudata.so`，将两者放置到`openarkcompiler/third_party/icu/lib/aarch64-linux-gnu`路径下，并重命名为`libicuuc.so.56`和`libicudata.so.56`。

### libz 下载并编译

当前用例编译需要 libz.so 支持，请使用 1.2.8 版本。

libz 下载地址：https://zlib.net

下载 1.2.8 版本的 libz.so，将其放置到`openarkcompiler/third_party/libdex/prebuilts/aarch64-linux-gnu/`路径下，并重命名为`libz.so.1.2.8`。

### r8 下载并编译

当前用例编译需要 d8.jar 支持，请使用 d8-1.5.13 版本。

r8 社区地址：https://r8.googlesource.com/r8

已经编译后的二进制：https://gitee.com/xlnb/r8-d81513/tree/master/d8/lib/d8.jar

将 d8.jar 放置到`openarkcompiler/third_party/d8/lib/`目录
