# Maple 测试框架

## 目录结构

```shell
test
├── main.py   运行测试套入口
├── maple_test    测试框架代码
│   ├── compare.py    结果校验模块
│   ├── configs.py    参数设置与框架配置文件模块
│   ├── __init__.py
│   ├── main.py    内部入口
│   ├── maple_test.cfg    测试框架配置文件
│   ├── run.py    命令运行模块
│   ├── task.py   测试任务准备与运行模块
│   ├── template.cfg    测试套配置文件模板
│   ├── test.py   测试用例模块
│   └── utils.py    通用模块
├── README.md   测试框架说明
└── testsuite
    ├── irbuild_test    irbuild测试套
    ├── ouroboros 	ouroboros测试套
    │	├──test.cfg		测试套配置文件
    │	├──test_android.cfg		测试套手机执行配置文件
    │	└──testlist		测试内容
    ├──maple.py		用例编译脚本
    ├──run.py	 用例执行脚本
    ├──android_maple.py		Android用例编译脚本
    └──android_run.py	 Android用例执行脚本
```

## 运行要求

- python版本>=3.5.2

## 修改框架配置

文件：maple_test.cfg

```ini
[test-home]
# 指定测试套路径，以‘：’划分
dir = ../testsuite/irbuild_test:../testsuite/ouroboros:

[running]
# 指定运行时的临时路径
temp_dir = ../test_temp/run

[logging]
# 指定运行时保存日志的路径
name = ../test_temp/log
level = INFO
```

## 运行说明

依赖环境变量：MAPLE_ROOT

### 运行测试套

#### irbuild_test

```
python3 test/main.py test/testsuite/irbuild_test --test_cfg=test/testsuit/irbuild_test/<test_cfg_file> -j20 -pFAIL
```

#### ouroboros

```
python3 test/main.py test/testsuite/ouroboros --test_cfg=test/testsuit/ouroboros/<test_cfg_file> -j20 -pFAIL
```

参数说明：指定参数会覆盖框架配置文件中的设置

```txt
usage: main.py [-h] [--cfg CFG] [-j <mum>] [--retry <num>] [--output <file>]
               [--debug] [--fail_exit]
               [-p {PASS,FAIL,XFAIL,XPASS,UNSUPPORTED,UNRESOLVED}]
               [--progress {silent,normal,no_flush_progress}]
               [--test_cfg <TEST_CFG_FILE>] [--test_list <TEST_LIST_FILE>]
               [-c config set path] [-C key=value] [-E key=value]
               [--temp_dir <TEMP_DIR_PATH>] [--timeout TIMEOUT]
               [--log_dir <LOG_DIR_FILE_PATH>] [--log_level LOG_LEVEL]
               [--verbose]
               [test_paths [test_paths ...]]

optional arguments:
  -h, --help            show this help message and exit

Test FrameWork arguments:
  --cfg CFG             Test framework configuration file
  -j <mum>              Run <num> cases in parallel
  --retry <num>         Re-run unsuccessful test cases
  --output <file>       Store test result at <file>
  --debug               keep test temp file
  --fail_exit           Execute test framework with a non-zero exit code if
                        any tests fail
  -p {PASS,FAIL,XFAIL,XPASS,UNSUPPORTED,UNRESOLVED}
                        Print test cases with specified results, -pPASS
                        -pFAIL, to print all test case that failed or passed，
                        UNRESOLVED test case results are not displayed by
                        default.
  --progress {silent,normal,no_flush_progress}
                        set progress type, silent: Don't show progress,
                        normal: one line progress bar, update per
                        second,no_flush_progress: print test progress per 10
                        seconds

Test Suite arguments:
  test_paths            Test suite path
  --test_cfg <TEST_CFG_FILE>
                        test suite config file, needed when run a single case
                        or with --test_list
  --test_list <TEST_LIST_FILE>
                        testlist path for filter test cases
  -c config set path, --config_set config set path
                        Run a test set with the specified config set path
  -C key=value, --config key=value
                        Add 'key' = 'val' to the user defined configs
  -E key=value, --env key=value
                        Add 'key' = 'val' to the user defined environment
                        variable

Running arguments:
  --temp_dir <TEMP_DIR_PATH>
                        Location for test execute.
  --timeout TIMEOUT     test case timeout

Log arguments:
  --log_dir <LOG_DIR_FILE_PATH>
                        Where to store test log
  --log_level LOG_LEVEL, -l LOG_LEVEL
                        set log level from: CRITICAL, ERROR, WARNING, INFO,
                        DEBUG, NOTSET
  --verbose             enable verbose output
```

## 测试框架编码支持说明

当前测试框架仅支持编码格式为 `UTF-8`，测试用例和配置文件仅支持 `UTF-8` 编码格式，如果测试用例编码非 `UTF-8` 测试用例会被认定为 `UNRESOLVED`

## ouroboros 测试套

ouroboros测试套是基于 `Java` 测试用例的测试套，当前ouroboros测试支持本地测试及系统版本为华为emui10的android手机测试。

### 测试环境准备

* 本地测试前请首先执行完成`make libcore`。

* android测试前请首先执行完成`make libcore OPS_ANDROID=1`。之后将`openarkcompiler/output/ops/host-x86_64-O2`中的`libcore-all.so`、`libmplandroid.so`和`libmplopenjdk.so`推送至测试手机的`/system/lib64/`下，文件名保持不变。将`openarkcompiler/output/ops/mplsh`推送至测试手机的`/system/bin/`下，并将文件重命名为`mplsh_arm64`。

### 运行测试套

* 参数`--test_cfg`设定选择的测试套配置，默认使用`test.cfg`进行本地测试，使用`test_android.cfg`将进行android版本的测试。

* 在运行android版本的测试前，需要在test_android.cfg中配置测试机所在服务器的ssh及测试机sn，并提前配置完成本地与服务器间的ssh互信。

批量运行ouroboros：参数 `-j20` 设定并行为20。

`python3 test/main.py test/testsuite/ouroboros/ --test_cfg=test/testsuite/ouroboros/<test_cfg_file> -j20`

运行ourobors下的子文件夹：

`python3 test/main.py test/testsuite/ouroboros/string_test --test_cfg=test/testsuite/ouroboros/<test_cfg_file> -j20`

运行ourobors下的单一测试用例：

`python3 test/main.py test/testsuite/ouroboros/string_test/RT0001-rt-string-ReflectString/ReflectString.java --test_cfg=test/testsuite/ouroboros/<test_cfg_file>`

只输出失败用例：

`python3 test/main.py test/testsuite/ouroboros/string_test/RT0001-rt-string-ReflectString/ReflectString.java --test_cfg=test/testsuite/ouroboros/<test_cfg_file> -pFAIL`

屏幕输出详细运行日志：

`python3 test/main.py test/testsuite/ouroboros/string_test/RT0001-rt-string-ReflectString/ReflectString.java --test_cfg=test/testsuite/ouroboros/<test_cfg_file> --verbose`

### 测试套配置

测试套配置文件路径在 `testsuite/ourobors/`下, 以`test.cfg`为例，其中含有测试套的一些设置和内部变量

```ini
[suffix]
java = //

[internal-var]
maple = <用例编译脚本>
run = <用运行脚本>
build_option = <编译脚本参数>
run_option = <运行脚本参数>

[description]
title = Maple Ouroboros Test
```

**`[suffix]`**：限定搜索测试用例文件的后缀，以及测试用例中注释符，注释符后会跟随执行语句或者校验语句，当前测试套中的用例为 `java` 文件，`//` 作为注释符。

**`[internal-val]`**：内部变量，此处的内部变量会替换用例中相应的变量。例如配置文件中的 `maple = python3 ${MAPLE_ROOT}/test/testsuite/maple.py` ，将会将用例中跟随在 `\\ EXEC: ` 之后的执行语句中的 `%maple` 替换为 `python3 ${MAPLE_ROOT}/test/testsuite/maple.py`。

**`[description]`**：测试套的描述信息。

### 测试套列表

默认测试列表路径为 `testsuite/ourobors/testlist`，测试列表规定了运行测试用例的范围，同时指定了排除的测试用例。

```ini
[ALL-TEST-CASE]
    arrayboundary_test
    clinit_test
    eh_test
    fuzzapi_test
    other_test
    parent_test
    reflection_test
    stmtpre_test
    string_test
    subsumeRC_test
    thread_test
    unsafe_test
    memory_management

[EXCLUDE-TEST-CASE]
    memory_management/Annotation
```

由两个部分组成：`[ALL-TEST-CASE]` 与 `[EXCLUDE-TEST-CASE]`

**`[ALL-TEST-CASE]`**: 指定了运行测试用例的范围。

**`[EXCLUDE-TEST-CASE]`**: 不运行的测试用例。

当前测试用例排除了测试套 `testsuite/ourobors` 下子文件夹 `memory_management/Annotation` 中所有的用例文件，及部分其他用例。


## irbuild测试套

irbuild用于对maple中端产物进行测试，仅进行本地测试，不支持手机测试。

irbuild测试套配置：testsuite/irbuild_test/test.cfg
如果涉及脚本的运行路径需要填写绝对路径或者在环境变量（PATH）中，例如：

配置文件中：如果cmp在PATH中，则 cmp = cmp 即可，如果不在则 cmp = /usr/bin/cmp

```ini
[suffix]
mpl = #

[internal-var]
irbuild = ${MAPLE_ROOT}/output/bin/irbuild
cmp = /usr/bin/cmp -s

[description]
title = Maple Irbuild Test
```

### 运行单个irbuild用例

```shell
python3 main.py -pFAIL -pPASS --timeout=180 --test_cfg=testsuite/irbuild_test/test.cfg testsuite/irbuild_test/I0001-mapleall-irbuild-edge-addf32
```

### 运行文件夹内的所有用例

```shell
python3 main.py -pFAIL -pPASS --timeout=180 --test_cfg=testsuite/irbuild_test/test.cfg testsuite/irbuild_test
```

### irbuild测试套文件结构

```shell
    testsuite/irbuild 测试套路径
    ├── I0001-mapleall-irbuild-edge-addf32
    │   ├── Main.mpl 测试用例
    ├── ...
    └── ...
```

### irbuild_test 测试用例说明

#### 完整Main.mpl

```
 func &addf32r(
  var %i f32, var %j f32
  ) f32 { 
   return (
     add f32(dread f32 %i, dread f32 %j))}

 func &addf32I (
  var %i f32
  ) f32 { 
   return (
     add f32(dread f32 %i,
       constval f32 1.234f))}
 # EXEC: %irbuild Main.mpl
 # EXEC: %irbuild Main.irb.mpl
 # EXEC: %cmp Main.irb.mpl Main.irb.irb.mpl
```

#### 1. 测试案例部分

```
 func &addf32r(
  var %i f32, var %j f32
  ) f32 { 
   return (
     add f32(dread f32 %i, dread f32 %j))}

 func &addf32I (
  var %i f32
  ) f32 { 
   return (
     add f32(dread f32 %i,
       constval f32 1.234f))}
```

#### 2. 测试案例运行部分

```
 # EXEC: %irbuild Main.mpl
 # EXEC: %irbuild Main.irb.mpl
 # EXEC: %cmp Main.irb.mpl Main.irb.irb.mpl
```

三条执行语句：

1. EXEC语句，利用%irbuild，编译Main.mpl为Main.irb.mpl
2. EXEC语句，利用%irbuild，编译Main.irb.mpl为Main.irb.irb.mpl
3. EXEC语句，利用%cmp，比较Main.irb.irb.mpl与Main.irb.mpl是否一致，一致测试通过