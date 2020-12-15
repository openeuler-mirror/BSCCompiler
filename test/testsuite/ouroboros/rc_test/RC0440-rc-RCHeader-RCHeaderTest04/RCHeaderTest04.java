/*
 *- @TestCaseID:RCHeaderTest04.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 软指针指向的对象声明周期未结束时，同时有cleaner referent和Weak Annotation referent引用的资源和对象是不会释放的
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：在方法体内为一个具有缓存语义的软引用rcht.sr加上一个Cleaner和一个Weak Annotation指向它；
 * -#step2：该软引用生命周期结束后，应该会立刻释放该对象占用的内存（Cleaner和Weak Annotation对象没有缓存语义）
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCHeaderTest04.java
 *- @ExecuteClass: RCHeaderTest04
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import sun.misc.Cleaner;

import java.lang.ref.SoftReference;

import com.huawei.ark.annotation.Weak;

class RCRunner04 implements Runnable {
    public void run() {
        RCHeaderTest04.iArray = null;
    }
}


public class RCHeaderTest04 {
    static int[] iArray;
    @Weak
    static int[] wr_annotation = null;
    static Cleaner c = null;
    static SoftReference<int[]> sr = null;

    public RCHeaderTest04() {
        iArray = new int[4];
        int[] temp = {1, 2, 3, 4};
        wr_annotation = temp;
        sr = new SoftReference<>(temp);
    }

    //方法退出后，实例变量sr声明周期结束，触发对SoftReference的处理，它指向的数组占用空间被释放。
    static void foo() {
        RCHeaderTest04 rcht = new RCHeaderTest04();
        c = Cleaner.create(rcht.sr, new RCRunner04());
        if (c == null && wr_annotation.length != 4) {
            System.out.println("Cleaner create failly");
        }
    }

    public static void main(String[] args) {
        foo();
        Runtime.getRuntime().runFinalization();
        if (c != null && wr_annotation != null && sr.get() != null) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}



// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
