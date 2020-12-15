/*
 *- @TestCaseID:RCHeaderTest02.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 软指针指向的对象生命周期结束后，仍及时回收cleaner referent引用的资源和对象
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：在方法体内为一个具有缓存语义的软引用rcht.sr加上一个Cleaner指向它；
 * -#step2：该软引用生命周期结束后，应该会立刻释放该对象占用的内存（Cleaner没有缓存语义）
 *- @Expect:ExpectResult\nExpectResultEnd\n
 *- @Priority: High
 *- @Source: RCHeaderTest02.java
 *- @ExecuteClass: RCHeaderTest02
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import sun.misc.Cleaner;
import java.util.ArrayList;
import java.lang.ref.SoftReference;

class RCRunner02 implements Runnable {
    public void run() {
        RCHeaderTest02.iArray = null;
    }
}

class Resource02 {

    @Override
    public void finalize() {
        System.out.println("ExpectResult");
    }
}


public class RCHeaderTest02 {
	private static ArrayList<byte[]> store;
    static int[] iArray;
    static Cleaner c = null;
    Resource02 res02 = null;

    public RCHeaderTest02() {
        iArray = new int[4];
        res02 = new Resource02();

    }

    //方法退出后，sr_rcht02声明周期结束，触发对SoftReference的处理，它指向的对象占用空间被释放。
    static void foo() {
        SoftReference<RCHeaderTest02> sr_rcht02 = new SoftReference<>(new RCHeaderTest02());
        c = Cleaner.create(sr_rcht02, new RCRunner02());
        if (sr_rcht02.get() == null || c == null) {
            System.out.println("SoftReference error or Cleaner create failly");
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
        foo();
        try {
            int oom = oomTest();
        } catch (OutOfMemoryError ofm) {}
        Runtime.getRuntime().runFinalization();
        System.out.println("ExpectResultEnd");    //ReleaseEnd
    }
}



// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResultEnd\n
