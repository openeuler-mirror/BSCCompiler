# 开发者指南

通过参考本文档，您可以下载编译器源码编译出OpenArkCompiler。同时，本文档也为开发者提供了源码静态检查指南。

## 源码下载

下载地址：<https://code.opensource.huaweicloud.com/HarmonyOS/OpenArkCompiler/home>，可以通过`Clone` or `Download`的方式下载openarkcompiler源码。

   > 注：默认源码下载目录为openarkcompiler。

之后请按照《环境配置》文档完成您的开发环境准备。


## 源码编译

在openarkcompiler目录下执行以下命令，编译出OpenArkCompiler，默认输出路径 openarkcompiler/output/TYPE/bin, TYPE: aarch64-clang-release。

```
source build/envsetup.sh arm release
make setup
make
```
**请注意：**近期升级语言前端工具链clang至版本15后：
```
在OpenArkCompiler同级目录ThirdParty下：git pull
如果遇到冲突，建议直接删除ThirdParty目录，在上述make setup阶段会重新拉取最新的ThirdParty
```



命令说明：

- `source build/envsetup.sh arm release` 初始化环境，将OpenArkCompiler工具链路径openarkcompiler/output/TYPE/bin设置到环境变量中；
- `make` 编译OpenArkCompiler的Release版本；
- `make BUILD_TYPE=DEBUG` 编译OpenArkCompiler的Debug版本。

在openarkcompiler目录下执行以下命令，编译出OpenArkCompiler及maple runtime部分，默认输出路径 openarkcompiler/output/TYPE, TYPE: aarch64-clang-release。

```
source build/envsetup.sh arm release
make setup
make libcore
```

命令说明：

- `make libcore` 编译OpenArkCompiler及maple runtime部分的Release版本；
- `make libcore BUILD_TYPE=DEBUG` 编译OpenArkCompiler及maple runtime部分的Debug版本；

此外，方舟编译器还提供了源码编译脚本，开发者也可以通过在openarkcompiler目录下执行该脚本，默认编译出OpenArkCompiler及maple runtime部分的Release版本。执行命令如下：

```
source build/build.sh
```

## Sample示例编译

当前编译方舟编译器Sample应用需要使用到Java基础库，我们以Android系统提供的Java基础库为例，展示Sample样例的编译过程。

**基础库准备**

环境准备阶段已经通过AOSP获取到需要的libcore的jar文件。

**生成libjava-core.mplt文件**

编译前，请先在openarkcompiler目录下创建libjava-core目录，拷贝java-core.jar到此目录下，在openarkcompiler目录执行以下命令：

```
source build/envsetup.sh arm release
make
cd libjava-core
jbc2mpl -injar java-core.jar -out libjava-core
```

执行完成后会在此目录下生成libjava-core.mplt文件。

**示例代码快速编译**

示例代码位于openarkcompiler/samples目录。

以samples/helloworld/代码为例，在openarkcompiler/目录下执行以下命令：

```
source build/envsetup.sh arm release
make
cd samples/helloworld/
make
```

## 源码静态检查

本部分内容将指导您使用clang-tidy进行源码静态检查。在对源码进行修改之后，对源码进行静态检查，可以检查源码是否符合编程规范，有效的提高代码质量。

静态源码检查之前，需要先编译出OpenArkCompiler。此后，以检查src/maple_driver源码为例，在openarkcompiler目录下执行以下命令：

```
cp output/TYPE/compile_commands.json ./
./tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/share/clang/run-clang-tidy.py -clang-tidy-binary='./tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang-tidy' -clang-apply-replacements-binary='./tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang-apply-replacements' src/maple_driver/
```

命令说明：

- `cp output/TYPE/compile_commands.json ./` 将output/TYPE目录之下的compile_commands.json复制到当前目录之下，它是clang-tidy运行所需要的编译命令；

- `./tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/share/clang/run-clang-tidy.py` 调用clang-tidy进行批量检查的脚本run-clang-tidy.py，其中 `./tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/`目录是之前配置的clang编译器的发行包主目录； `-clang-tidy-binary` 是指明clang-tidy的具体位置； `-clang-apply-replacements-binary` 是指明run-clang-tidy.py所依赖的clang-apply-replacements的位置； `src/maple_driver/` 是要进行源码检查的目录。

## 编译器注意事项

- 方舟编译器前端暂时不支持字节码校验，未符合dex、jbc语义的字节码输入可能造成编译器Crash。
- 方舟编译器中后端暂时不支持IR中间件语法校验，未符合MapleIR语义的中间件输入可能造成编译器Crash。
