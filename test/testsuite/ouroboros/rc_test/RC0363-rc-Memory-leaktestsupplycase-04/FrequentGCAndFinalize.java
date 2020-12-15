/*
 *- @TestCaseID: maple/runtime/rc/function/FrequentGCAndFinalize.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建一个WeakReference类的对象rp，经判断rp.get()的返回值不为null；
 * -#step2: 令stringBuffer等于null；
 * -#step3: 创建一个ThreadFinalize类的实例对象threadFinalize，对于i < num = 10成立时，对其调用run()方法，即调用
 *          System.runFinalization()方法，对失去引用的对象调用finalize()方法进行资源的释放；
 * -#step4: 创建一个ThreadGC类的实例对象threadGC，对于i < num = 10成立时，对其调用run()方法，即调用
 *          Runtime.getRuntime().gc()进行垃圾回收；
 * -#step5: 重复step3；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: FrequentGCAndFinalize.java
 *- @ExecuteClass: FrequentGCAndFinalize
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class FrequentGCAndFinalize {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int num = 10;
    static StringBuffer stringBuffer = new StringBuffer("weak");

    static void setWeakRef() {
        stringBuffer = new StringBuffer("weak");
        rp = new WeakReference<Object>(stringBuffer, rq);
        if (rp.get() == null) {
            System.out.println("error");
        }
        stringBuffer = null;
    }

    static void executeThreadGC() {
        ThreadGC threadGC = new ThreadGC();
        for (int i = 0; i < num; i++) {
            threadGC.run();
        }
    }

    static void executeThreadFinalize() {
        ThreadFinalize threadFinalize = new ThreadFinalize();
        for (int i = 0; i < num; i++) {
            threadFinalize.run();
        }
    }

    public static void main(String[] args) {
        setWeakRef();
        executeThreadFinalize();
        executeThreadGC();
        executeThreadFinalize();
        System.out.println("ExpectResult");
    }
}

class ThreadGC implements Runnable {
    @Override
    public void run() {
        Runtime.getRuntime().gc();
    }
}

class ThreadFinalize implements Runnable {
    @Override
    public void run() {
        ThreadFinalize threadFinalize = new ThreadFinalize();
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
