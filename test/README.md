# Maple 测试框架

## 目录结构

```shell
test
├── main.py   运行测试套入口
├── maple_test    测试框架代码
│   ├── compare.py    结果校验模块
│   ├── configs.py    参数设置与框架配置文件模块
│   ├── __init__.py
│   ├── main.py    内部入口
│   ├── maple_test.cfg    测试框架配置文件
│   ├── run.py    命令运行模块
│   ├── task.py   测试任务准备与运行模块
│   ├── template.cfg    测试套配置文件模板
│   ├── test.py   测试用例模块
│   └── utils.py    通用模块
├── README.md   测试框架说明
└── testsuite
    └── irbuild_test    irbuild测试套
```

## 运行要求

* python版本>=3.5.2

## 修改框架配置

文件：maple_test.cfg

```ini
[test-home]
# 指定测试套路径，以‘：’划分
dir =
    ../testsuite/irbuild_test:

[running]
#指定运行时的临时路径
temp_dir = ../test_temp/run

[logging]
#指定运行时保存日志的路径
name = ../test_temp/log
level = INFO
```

## 运行说明

依赖环境变量：MAPLE_ROOT

### 运行单个irbuild用例

```shell
python3 main.py -pFAIL -pPASS --timeout=180 --test_cfg=testsuite/irbuild_test/test.cfg testsuite/irbuild_test/I0001-mapleall-irbuild-edge-addf32
```

### 运行文件夹内的所有用例

```shell
python3 main.py -pFAIL -pPASS --timeout=180 --test_cfg=testsuite/irbuild_test/test.cfg testsuite/irbuild_test
```

### 运行已配置的所有测试套

```shell
python3 test/main.py -j20 --timeout=120
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

## irbuild测试套配置说明

irbuild测试套配置：testsuite/irbuild_test/test.cfg
如果涉及脚本的运行路径需要填写绝对路径或者在环境变量（PATH）中
例如配置文件中：如果cmp在PATH中，则 cmp = cmp 即可，如果不在则 cmp = /usr/bin/cmp

```ini
[suffix]
mpl = #

[internal-var]
irbuild = ${MAPLE_ROOT}/output/bin/irbuild
cmp = /usr/bin/cmp -s

[description]
title = Maple Irbuild Test
```

### suffix说明

```ini
[suffix]
mpl = #
```

* 测试用例以"mpl"作为文件后缀
* 文件后缀"mpl"的测试用例内以"#"作为注释符

### 内部变量说明

```ini
[internal-var]
irbuild = ${MAPLE_ROOT}/output/bin/irbuild
cmp = /usr/bin/cmp -s
```

* 所有测试用例中的EXEC语句内的"%irbuild"会被替换为"${MAPLE_ROOT}/output/bin/irbuild"
* 所有测试用例中的EXEC语句内的"%cmp"会被替换为"/usr/bin/cmp -s"

#### irbuild测试套文件结构

```shell
    testsuite/irbuild 测试套路径
    ├── I0001-mapleall-irbuild-edge-addf32
    │   ├── Main.mpl 测试用例
    ├── ...
    └── ...
```

## 测试用例说明

### 完整Main.mpl

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
