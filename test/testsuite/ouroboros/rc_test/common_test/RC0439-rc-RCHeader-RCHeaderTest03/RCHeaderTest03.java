/*
 *- @TestCaseID:RCHeaderTest03.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 软指针指向的对象声明周期结束后，cleaner referent和Weak Annotation referent引用的资源和对象是会释放的
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：在方法体内为一个具有缓存语义的软引用rcht.sr加上一个Cleaner和一个Weak Annotation指向它；
 * -#step2：该软引用生命回收后，应该会立刻释放该对象占用的内存（Cleaner和Weak Annotation对象没有缓存语义）
 *- @Expect:oom occured\nSR release\nExpectResult\n
 *- @Priority: High
 *- @Source: RCHeaderTest03.java
 *- @ExecuteClass: RCHeaderTest03
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import sun.misc.Cleaner;
import java.util.ArrayList;
import java.lang.ref.SoftReference;

import com.huawei.ark.annotation.Weak;

class RCRunner03 implements Runnable {
    public void run() {
        RCHeaderTest03.iArray = null;
    }
}


public class RCHeaderTest03 {
    static String[] iArray;
    private static ArrayList<byte[]> store;
    String @Weak [] wr_annotation = null;
    Cleaner c = null;
    SoftReference<String[]> sr = null;

    public RCHeaderTest03() {
        iArray = new String[4];
        String[] temp = {"1", "2", "3", "4"};
        wr_annotation = temp;
        sr = new SoftReference<>(temp);
    }

    //方法退出后，实例变量sr声明周期结束，触发对SoftReference的处理，它指向的数组占用空间被释放。
    void foo() {
        c = Cleaner.create(wr_annotation, new RCRunner03());
        if (c == null || wr_annotation.length != 4) {
            System.out.println("Cleaner create failly");
        }
    }

    private static int oomTest() {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;

        for (int i = 1024 * 1024; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }

    public static void main(String[] args) {
        RCHeaderTest03 rcht = new RCHeaderTest03();
        rcht.foo();
        try {
            int oom = oomTest();
        } catch (OutOfMemoryError ofm) {
            System.out.println("oom occured");
        }
        Runtime.getRuntime().gc();
        if (rcht.sr.get() == null ) {
            System.out.println("SR release");
        }
        try {
            Thread.sleep(5000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (rcht.wr_annotation == null) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full oom occured\nSR release\nExpectResult\n
