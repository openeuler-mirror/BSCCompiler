/*
 *- @TestCaseID:RCHeaderTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 可以在不触发RP的情况下，及时回收cleaner referent引用的资源和对象
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：在方法体内为一个强引用加上一个Cleaner指向它；
 * -#step2：强引用声明周期结束后，可以在不触发RP的情况下，应该会被立刻释放该对象占用的内存（Cleaner没有缓存语义）
 *- @Expect:ExpectResult\nExpectResultEnd\n
 *- @Priority: High
 *- @Source: RCHeaderTest.java
 *- @ExecuteClass: RCHeaderTest
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import sun.misc.Cleaner;

class RCRunner implements Runnable {
    public void run() {
        RCHeaderTest.iArray = null;
    }
}

class Resource {

    @Override
    public void finalize() {
        System.out.println("ExpectResult");
    }
}


public class RCHeaderTest {
    static int[] iArray;
    static Cleaner c = null;
    Resource res;

    public RCHeaderTest() {
        iArray = new int[4];
        res = new Resource();
    }

    static void foo() {
        RCHeaderTest rcht = new RCHeaderTest();
        c = Cleaner.create(rcht, new RCRunner());
        if (c == null) {
            System.out.println("Cleaner create failly");
        }
    }

    public static void main(String[] args) {
        foo();
        Runtime.getRuntime().runFinalization();
        System.out.println("ExpectResultEnd");    //ReleaseEnd

    }
}


// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResultEnd\n
