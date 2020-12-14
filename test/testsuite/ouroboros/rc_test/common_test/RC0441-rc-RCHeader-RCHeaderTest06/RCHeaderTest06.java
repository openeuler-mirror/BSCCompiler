/*
 *- @TestCaseID:RCHeaderTest06.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 可以在不触发RP的情况下，及时回收Weak Annoatation referent和Cleaner 共同引用的资源和Finalize对象
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：在方法体内为一个强引用加上一个Weak Annoatation referent指向它；
 * -#step2：强引用声明周期结束后，可以在不触发RP的情况下，应该会被立刻释放该对象占用的内存（Weak Annoatation referent没有缓存语义）
 *- @Expect:ExpectResult\nExpectResultEnd\n
 *- @Priority: High
 *- @Source: RCHeaderTest06.java
 *- @ExecuteClass: RCHeaderTest06
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import sun.misc.Cleaner;

import com.huawei.ark.annotation.Weak;
class RCRunner06 implements Runnable {
    public void run() {
        RCHeaderTest06.iArray = null;
    }
}

class Resource06 {


    @Override
    public void finalize() {
        System.out.println("ExpectResult");
    }
}


public class RCHeaderTest06 {
    static int[] iArray;
    static Cleaner cleaner = null;
    @Weak
    Resource06 c = null;

    public RCHeaderTest06() {
        iArray = new int[4];
        c = new Resource06();
    }

    static void foo() {
        RCHeaderTest06 rcht = new RCHeaderTest06();
        RCHeaderTest06.cleaner = Cleaner.create(rcht.c, new RCRunner06());
        if (cleaner == null) {
            System.out.println("Weak Annotation Object shouldn't be null");
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
