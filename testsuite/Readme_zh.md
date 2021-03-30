# Maple 测试框架

## 目录结构

```shell
testsuite
├── README_zh.md    测试框架中文说明文档
├── driver    测试框架代码
│   ├── config    测试套集合配置
|   |   └── testall.conf    testall target 测试套集合配置
│   ├── src    测试框架源码 
|   |   ├── api    常用测试命令封装
|   |   |   ├── shell_operator.py    shell命令封装父类
|   |   |   ├── shell.py    一般shell封装
|   |   |   ├── maple.py    maple 可执行二进制封装
|   |   |   ├── dex2mpl.py    dex2mpl 可执行二进制封装
|   |   |   ├── ...  (可根据需求自行添加)
|   |   |   └── ... 
|   |   ├── basic_tools    测试框架常用的业务无关的函数库
|   |   |   ├── file.py    文件函数库
|   |   |   ├── string_list.py    字符串列表函数库
|   |   |   └── string.py    字符串函数库
|   |   ├── case.py    测试用例封装类
|   |   ├── driver.py    测试框架入口
|   |   ├── env_var.py    测试环境变量处理
|   |   ├── mod    测试用例的测试流程模式配置
|   |   |   ├── O0.py    测试用例使用编译器O0选项对应的编译和运行测试流程模式配置
|   |   |   ├── O2.py    测试用例使用编译器O2选项对应的编译和运行测试流程模式配置
|   |   |   ├── ...  (可根据需求自行添加)
|   |   |   └── ...
|   |   ├── mod_table.py    测试流程模式封装类
|   |   ├── shell_executor.py    单独一条shell命令执行封装
|   |   ├── target    测试套封装
|   |   ├── case.py    测试用例封装
|   |   ├── task.py    测试用例执行任务封装
|   |   └── test_cfg.py    测试用例封装类      ​
│   └── script    测试框架接口命令脚本
├── irbuild_test    irbuild测试套
├── java_test    java测试套
├── irbuil_test    irbuild测试套
└── c_test    c测试套

```

## 运行要求

- python版本>=3.5.2

### 运行说明

设置环境变量：source build/envsetup.sh arm release

测试套集合配置文件在testsuite/driver/config目录，里面可以添加多个测试套集合配置文件，每个配置文件包含流程模式集合，指定测试套能运行的流程模式集合，ban列表，配置文件语法如下：

流程模式集合 [OPTION_SUITE]
    mod_suite_name: mod1, mod2    将mod1 mod2两个流程模式封装为mod_suite_name
如 common_option: O0，O2，即将方舟编译器O0，O2模式封装到常用测试模式流程集合common_option中，后续如有新模式可继续添加

指定测试套能运行的流程模式集合 [DEFAULT_SUITE]
    java_test/app_test: common_option    testsuite下java_test/app_test路径下的case都能跑common_option中的模式流程，等价于：java_test/app_test: O0, O2
支持流程模式集合和流程模式同时使用，如：
    java_test/app_test: common_option， special_mod
支持路径配置覆盖，如：
    java_test/app_test/APP0001-app-Application-Application-helloworld: O3, O4     即这个case只能跑O3，O4，而不是跑java_test/app_test指定的流程模式集合

ban列表 [BAN_SUITE]
    基于指定测试套

说明：
1.测试套路径全部是相对testsuite的真实存在的路径，且必须是文件夹
2.流程模式必须是在testsuite/driver/mod中存在的已配置的流程模式，否则会报错