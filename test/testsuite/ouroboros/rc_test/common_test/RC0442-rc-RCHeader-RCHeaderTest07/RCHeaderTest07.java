/*
 *- @TestCaseID:RCHeaderTest07.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 可以在不触发RP的情况下，及时回收cleaner referent引用的资源和Finalize对象
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：在方法体内为一个强引用的finalize对象域加上一个Cleaner指向它；
 * -#step2：强引用声明周期结束后，可以在不触发RP的情况下，应该会被立刻释放该对象占用的内存（Cleaner没有缓存语义）
 *- @Expect:ExpectResult\nExpectResultEnd\n
 *- @Priority: High
 *- @Source: RCHeaderTest07.java
 *- @ExecuteClass: RCHeaderTest07
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import sun.misc.Cleaner;
class RCRunner07 implements Runnable {
    public void run() {
        RCHeaderTest07.iArray = null;
    }
}

class Resource07 {


    @Override
    public void finalize() {
        System.out.println("ExpectResult");
    }
}


public class RCHeaderTest07 {
    static int[] iArray;
    static Cleaner c = null;
    Resource07 res;

    public RCHeaderTest07() {
        iArray = new int[4];
        res = new Resource07();
    }

    static void foo() {
        RCHeaderTest07 rcht = new RCHeaderTest07();
        c = Cleaner.create(rcht.res, new RCRunner07());
        if (c == null) {
            System.out.println("Cleaner create failly");
        }
    }

    public static void main(String[] args) {
        foo();
        Runtime.getRuntime().runFinalization();
        System.out.println("ExpectResultEnd");
    }
}



// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResultEnd\n
