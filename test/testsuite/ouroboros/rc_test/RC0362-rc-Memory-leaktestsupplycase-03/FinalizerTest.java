/*
 *- @TestCaseID: maple/runtime/rc/function/FinalizerTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *                      等finalizer队列中所有对象执行finalize()方法，确认所有finalize资源顺利释放
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建一个FinalizerTest类的实例对象finalizerTest，对其调用newFinalizeObject()方法，即先创建一个WeakReference类的对
 *          象并赋值给rp[i]（此处i < finalizeNum = 10），经判断得知rp[i].get()的返回值不为null，最后令stringBuffer等于null；
 * -#step2: 重复步骤1九次；
 * -#step3: 令静态的全局变量finalizerTest等于null；
 * -#step4: 使当前线程休眠2000ms；
 * -#step5: 调用System.gc()进行垃圾回收；
 * -#step6: 调用System.runFinalization()方法，即强制调用失去引用的对象的finalize()方法进行资源的释放；
 * -#step7: 经判断得出rp[i].get()（此处i < finalizeNum = 10）的返回值均为null，证实对象已经完全释放；
 * -#step8: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step9: 重复步骤1~7；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerTest.java
 *- @ExecuteClass: FinalizerTest
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class FinalizerTest extends Object {
    static int finalizeNum = 10;
    static Reference rp[] = new Reference[finalizeNum];
    static ReferenceQueue rq = new ReferenceQueue();
    static int checkNum = 0;
    static StringBuffer stringBuffer;
    static FinalizerTest finalizerTest = null;

    static void newFinalizeObject() throws InterruptedException {
        stringBuffer = new StringBuffer("weak");
        for (int i = 0; i < finalizeNum; i++) {
            rp[i] = new WeakReference<Object>(stringBuffer, rq);
            if (rp[i].get() == null) {
                checkNum--;
            }
        }
        stringBuffer = null;
    }

    protected void finalize() throws Throwable {
        super.finalize();
        checkNum++;
    }

    static void newFinalizeTest() throws InterruptedException {
        for (int i = 0; i < finalizeNum; i++) {
            FinalizerTest finalizerTest = new FinalizerTest();
            finalizerTest.newFinalizeObject();
        }
        finalizerTest = null;
    }

    static void finalizeTest() throws InterruptedException {
        newFinalizeTest();
        Thread.sleep(2000);
        System.gc();
        System.runFinalization();
        for (int i = 0; i < finalizeNum; i++) {
            if (rp[i].get() != null) {
                checkNum--;
            }
        }
    }

    public static void main(String[] args) throws InterruptedException {
        finalizeTest();
        Runtime.getRuntime().gc();
        finalizeTest();

        if (checkNum == (finalizeNum * 2)) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("checkNum = " + checkNum);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
