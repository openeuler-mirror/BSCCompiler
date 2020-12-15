/*
 *- @TestCaseID: maple/runtime/rc/function/ref_processor_strong_and_weak_ref.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Reference Processor WCB test: 1.if WCB == 0, return object value
 *                                                    2.if WCB == 1, return null
 *                                                    3.check change WCB 1 to 0
 *                                                    4.check change WCB 0 to 1
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建WeakReference类型的引用变量sr，wr和pr，经判断它们均不为null，令变量stringBuffer1、stringBuffer2、
 *          stringBuffer3等于null；
 * -#step2: 让当前线程休眠1000ms；
 * -#step3: 让当前线程休眠2000ms；
 * -#step4: 经判断sr.get()的返回值不为null，则令checkCount的值加1；同理，wr.get()的返回值也不为null，令checkCount的值
 *          加1；pr.get()的返回值也不为null，令checkCount的值加1；
 * -#step5: 分别令wrq.poll()、srq.poll()、prq.poll()的返回值为wrqPoll、srqPoll、prqPoll，经判断它们均不为null，并且都是
 *          WeakReference类的一个实例对象；
 * -#step6: 重复步骤1~5；
 * -#step7: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step8: 重复步骤1~6；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: Ref_Processor_Wcb_Test_Ref.java
 *- @ExecuteClass: Ref_Processor_Wcb_Test_Ref
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.*;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class Ref_Processor_Wcb_Test_Ref {
    static int checkCount = 0;
    static final int TEST_NUM = 1;
    static Reference wr, sr, pr;
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static StringBuffer stringBuffer1 = new StringBuffer("soft");
    static StringBuffer stringBuffer2 = new StringBuffer("weak");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");

    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();

        if (checkCount == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("errorResult ---- checkcount: " + checkCount);
        }
    }

    private static void test() {
        // test: if WCB == 0, return object value
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
            sleep(1000);
        }
        // test: if WCB == 1, return null
        checkRet("test step 2");
        // test: check change WCB 1 to 0
    }

    private static void test_01() {
        /*if WCB == 0, return object value except PhantomReference*/
        stringBuffer1 = new StringBuffer("soft");
        stringBuffer2 = new StringBuffer("weak");
        stringBuffer3 = new StringBuffer("phantom");

        wr = new WeakReference<Object>(stringBuffer2, wrq);
        if (wr.get() == null) {
            checkCount++;
        }

        sr = new WeakReference<Object>(stringBuffer1, srq);
        if (sr.get() == null) {
            checkCount++;
        }

        pr = new PhantomReference<Object>(stringBuffer3, prq);
        if (pr.get() != null) {
            checkCount++;
        }
        stringBuffer2 = null;
        stringBuffer1 = null;
        stringBuffer3 = null;
    }

    private static void checkRet(String funName) {
        sleep(2000);
        Reference wrqPoll, srqPoll, prqPoll;
        if (sr.get() != null) {
            checkCount++;
        }
        if (wr.get() != null) {
            checkCount++;
        }
        if (pr.get() != null) {
            checkCount++;
        }
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in wrq.poll()");
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in srq.poll()");
            }
        }
        while ((prqPoll = prq.poll()) != null) {
            if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                checkCount++;
                System.out.println(" ErrorResult in prq.poll()");
            }
        }
    }

    private static void sleep(int sleepNum) {
        try {
            Thread.sleep(sleepNum);
        } catch (InterruptedException e) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " sleep was Interrupted");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
