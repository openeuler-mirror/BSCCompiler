/*
 *- @TestCaseID: maple/runtime/rc/function/Ref_Processor_Mutator_Ref.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Reference Processor and Mutator threads are called simultaneously (test work queue)
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建三个ReferenceQueue类的对象，并令其分别等于wrq、srq、prq；声明三个StringBuffer类型的变量stringBuffer1、
 *          stringBuffer2、stringBuffer3并分别赋值为weak、soft、phantom；
 * -#step2: 以stringBuffer1、wrq为参数，创建一个WeakReference类的对象wr；同理，以stringBuffer2、srq为参数，创建一个
 *          WeakReference类的对象sr；以stringBuffer3、prq为参数，创建一个PhantomReference类的对象pr；
 * -#step3: 通过wr.get()、sr.get()、pr.get()方法分别获取各自的referent对象，经判断获取到的referent对象均不为null；
 * -#step4: 令变量stringBuffer1、stringBuffer2、stringBuffer3等于null；
 * -#step5: 让当前线程休眠2000ms；
 * -#step6: 经判断通过wr.get()方法获取的referent对象为null；同理，通过sr.get()方法获取的referent对象为null；
 * -#step7: wrq、srq、prq引用队列判断它们均为null；
 * -#step8: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step9: 重复步骤1~8三次；
 * -#step10: 重复步骤1~7；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: Ref_Processor_Mutator_Ref.java
 *- @ExecuteClass: Ref_Processor_Mutator_Ref
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.*;

public class Ref_Processor_Mutator_Ref {
    static int check_count = 0;
    static final int TEST_NUM = 1;
    static Reference wr, sr, pr;
    static ReferenceQueue wrq, srq, prq;
    static StringBuffer stringBuffer1 = new StringBuffer("weak");
    static StringBuffer stringBuffer2 = new StringBuffer("soft");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");

    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();

        if (check_count == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error,check_count: " + check_count);
        }
    }

    private static void test() {
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
            sleep(2000);
            if (wr.get() != null) {
                check_count++;
            }
            if (sr.get() != null) {
                check_count++;
            }
        }

        Reference wrqPoll, srqPoll, prqPoll;
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                check_count++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in " +
                        "wrq.poll()");
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                check_count++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in " +
                        "srq.poll()");
            }
        }
        while ((prqPoll = prq.poll()) != null) {
            if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                check_count++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in " +
                        "prq.poll()");
            }
        }
    }

    public static void test_01() {
        wrq = new ReferenceQueue();
        srq = new ReferenceQueue();
        prq = new ReferenceQueue();
        stringBuffer1 = new StringBuffer("weak");
        stringBuffer2 = new StringBuffer("soft");
        stringBuffer3 = new StringBuffer("phantom");
        wr = new WeakReference<StringBuffer>(stringBuffer1, wrq);
        if (wr.get() == null) {
            check_count++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ----------------------" +
                    "check_count: " + check_count);
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in wr");
        }

        sr = new WeakReference<StringBuffer>(stringBuffer2, srq);
        if (sr.get() == null) {
            check_count++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ----------------------" +
                    "check_count: " + check_count);
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in sr");
        }

        pr = new PhantomReference<StringBuffer>(stringBuffer3, prq);
        if (pr.get() != null) {
            check_count++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ----------------------" +
                    "check_count: " + check_count);
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in pr");
        }
        stringBuffer1 = null;
        stringBuffer2 = null;
        stringBuffer3 = null;
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
