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
|   |   ├── mode    测试用例的测试流程模式封装
|   |   |   ├── O0.py    测试用例使用编译器O0选项对应的编译和运行测试流程模式封装
|   |   |   ├── O2.py    测试用例使用编译器O2选项对应的编译和运行测试流程模式封装
|   |   |   ├── ...  (可根据需求自行添加)
|   |   |   └── ...
|   |   ├── mode_table.py    测试套集合配置解析类
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

## 运行说明

必须先设置环境变量：source build/envsetup.sh arm release

### 批量运行

在MAPLE_ROOT下，即代码根目录下，执行如：

    make testall  批量跑testsuite/driver/config/testall.conf指定的所有测试套


    make testall MODE=O2    批量跑testsuite/driver/config/testall.conf指定的所有测试套中支持O2流程模式的部分


    make java_test/app_test    批量跑测试套java_test/app_test，这里测试套名称可以是相对testsuite的相对路径


    make java_test/app_test MODE=O02     批量跑测试套java_test/app_test中支持O2流程模式的部分



支持自定义测试套，在testsuite/driver/config下，写测试套配置文件，文件名必须以test开头，便于和编译target区分，语法见后文[测试套配置]，之后可以在MAPLE_ROOT下，执行如：

    make [target_name] [MODE=mod]     



### 调试模式运行

比如要调试java_test/app_test/APP0001-app-Application-Application-helloworld，则切换路径到这个case下面，执行：

    mm MODE=O2

执行的流程模式自选，如不清楚这个case支持具体哪些测试模式，可以输入一个错误的模式名称，如mm MODE=xx，这样会提示支持的模式都有哪些。

执行的命令行会打印出来，可以复制后直接在当前路径下粘贴执行，便于调试。

清理调试运行生成的文件，执行：

    mm clean


## 测试套配置

### 按测试套集合配置文件



测试套集合配置文件在testsuite/driver/config目录，里面可以添加多个测试套集合配置文件，每个配置文件包含流程模式集合，指定测试套能运行的流程模式集合，ban列表，配置文件语法如下：


#### 流程模式集合 [MODE_SET]

    mode_set_name: mode1, mode2    将mode1 mode2两个流程模式封装为mode_set_name

如 java_common_mode_set: O0，O2，即将方舟编译器O0，O2模式封装到常用测试模式流程集合java_common_mode_set中，后续如有新模式可继续添加




#### 指定测试套能运行的流程模式集合 [DEFAULT_TEST_SUITE]

    java_test/app_test: java_common_mode_set    testsuite下java_test/app_test路径下的case都能跑java_common_mode_set中的模式流程，等价于：java_test/app_test: O0, O2


支持流程模式集合和流程模式同时使用，如：

    java_test/app_test: java_common_mode_set， special_mode


支持路径配置覆盖，如：

    java_test/app_test/APP0001-app-Application-Application-helloworld: O3, O4     即这个case只能跑O3，O4，而不是跑java_test/app_test指定的流程模式集合




#### ban列表 [BAN_TEST_SUITE]

基于指定测试套默认的能运行的流程模式集合[DEFAULT_TEST_SUITE]中，将不能运行的模式注释掉的ban列表，如果cmp在PATH中，如：
	
	java_test/app_test/APP0001-app-Application-Application-helloworld: O0     这一行声明这个case O0模式不跑，那么这个case将跑除了O0之外的测试套中指定的流程模式集合[DEFAULT_TEST_SUITE]

同样的测试套可以多次出现在ban列表中，ban列表中测试套注释掉的模式为多行的并集。
	

说明：

1.测试套路径全部是相对testsuite的真实存在的路径，且必须是文件夹

2.流程模式必须是在testsuite/driver/mode中存在的已配置的流程模式

3.配置文件中，支持#注释和空行


## 流程模式配置

流程模式配置文件在路径 testsuite/driver/src/mode 下，每一个流程模式配置文件里有且只有一个dict，dict 的变量名即为这个流程模式的名字，且一定与改流程模式配置文件的名字保持一致，否则无法正确加载，该文件需符合python3语法。

流程模式的dict封装结构如下：

    mode_name = {
        phase_name1 : [
		    api1，
			api2，
			...
		]，
		phase_name2 : [
		    api1，
			api2，
			...
		]，
		...
	}

其中，dict的key值为要封装的一个阶段流程的名称，如 clean compile run等，而对应的value为由若干个api组成的流程描述。

常用的api，建议封装在testsuite/driver/src/api 下面，每个api尽可能抽象并与业务解耦，这样才能最大程度复用，可变内容在实例化时用入参传入，在api解析时拼装成一条完整的shell命令

不常用的api，提供最原始的Shell api封装,可以直接写shell命令


## 用例配置

用例所在文件夹命名规范为^[A-Z]{1,9}[0-9]{3,10}-[a-zA-Z0-9_.]，1到9个大写字母加3到10位数字组成用例文件夹开头，后面用“-”链接若干由大小写字母，数字，'_' , '.'组成的字符串

用例所在文件夹下有test.cfg，即用例对应的配置文件，配置文件语法为：
    
	phase_name1()
	phase_name2(var1=1,var2="test")
	...

调用封装描述case测试的主干流程，phase_name 要存在于 上文流程模式配置文件里dict封装中的key中，()里为case相关的入参，格式为

    [var_name]=[var_value]

多个入参用','相连，无入参时()不可缺省

另 主类名传参时，如
    
    compile(APP=HelloWorld)

可简写为

    compile(HelloWorld)
