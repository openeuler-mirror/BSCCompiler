/*
 *- @TestCaseID:maple/runtime/rc/function/FinalizerRef.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建弱引用，确认创建成功，释放强引用。
 * -#step2: 创建5个线程调用finalize。
 * -#step3: 调用Runtime.getRuntime().gc()进行垃圾回收。
 * -#step4: 重复步骤1~2。
 *- @Expect:
 *- @Priority: High
 *- @Source: FinalizerRef.java
 *- @ExecuteClass: FinalizerRef
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class FinalizerRef {
    static int NUM = 1;

    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        System.out.println("ExpectResult");
    }

    public static void test() {
        SetWeakRef setWeakRef = new SetWeakRef();
        setWeakRef.setWeakRef();
        ThreadWeakRef_01 threadWeakRef_01 = new ThreadWeakRef_01();
        ThreadWeakRef_02 threadWeakRef_02 = new ThreadWeakRef_02();
        ThreadWeakRef_03 threadWeakRef_03 = new ThreadWeakRef_03();
        ThreadWeakRef_04 threadWeakRef_04 = new ThreadWeakRef_04();
        ThreadWeakRef_05 threadWeakRef_05 = new ThreadWeakRef_05();
        for (int i = 0; i < NUM; i++) {
            threadWeakRef_01.run();
            threadWeakRef_02.run();
            threadWeakRef_03.run();
            threadWeakRef_04.run();
            threadWeakRef_05.run();
        }
    }
}

class SetWeakRef {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer obj = new StringBuffer("weak");

    static void setWeakRef() {
        obj = new StringBuffer("weak");
        rp = new WeakReference(obj, rq);
        if (rp.get() == null) {
            System.out.println("error");
        }
        obj = null;
    }
}

class ThreadWeakRef_01 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_01 threadWeakRef_01 = new ThreadWeakRef_01();
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }
}

class ThreadWeakRef_02 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_02 threadWeakRef_02 = new ThreadWeakRef_02();
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }
}

class ThreadWeakRef_03 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_03 threadWeakRef_03 = new ThreadWeakRef_03();
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }
}

class ThreadWeakRef_04 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_04 threadWeakRef_04 = new ThreadWeakRef_04();
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }
}

class ThreadWeakRef_05 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_05 threadWeakRef_05 = new ThreadWeakRef_05();
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
