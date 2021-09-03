## Environment Configuration

## Recommended Hardware:

- 2 GHz dual-core processor or higher

- 2 GB system memory or higher

- 200 GB available disk space

## Recommended Development Environment

Install a 64-bit Ubuntu (Ubuntu 16.04, 18.04 or 20.04 is required).

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

## Auto Installation of Tools

```
source build/envsetup.sh arm release
make setup

Note: the following steps are for reference only. All required tools are installed during above "make setup"
```

## Installing and Configuring Clang (for Compiling the OpenArkCompiler Code)

Download **clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04**
LLVM download address: http://releases.llvm.org/download.html#10.0.0

Place the downloaded files in the `openarkcompiler/tools` directory, open the `openarkcompiler/build/config.gni` file, and set the three variables `GN_C_COMPILER`, `GN_CXX_COMPILER`, and `GN_AR_COMPILER` to the path where Clang is located. For example:

```
GN_C_COMPILER = "${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang"
GN_CXX_COMPILER = "${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang++"
GN_AR_COMPILER = "${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/llvm-ar"
```

${MAPLE_ROOT} is the root directory of the OpenArkCompiler source code.

## Installing and configuring Ninja and GN

Download **Ninja(v1.10.0)** and **GN(Linux Version)**
Ninja download address: https://github.com/ninja-build/ninja/releases
GN download address: https://gitee.com/xlnb/gn_binary

Place the executable programs of GN and Ninja in the openarkcompiler/tools directory, modify these two files to be executable.

```
cd openarkcompiler/tools
chmod 775 gn
chmod 775 ninja
```

Open the openarkcompiler/Makefile file, and set the two variables GN and NINJA to the path where the executable programs of GN and Ninja are located. For example,

```
GN := ${MAPLE_ROOT}/tools/gn/gn
NINJA := ${MAPLE_ROOT}/tools/ninja/ninja
```
