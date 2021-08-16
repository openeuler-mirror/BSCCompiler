/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest04
 *- @TestCaseName: GCOnly_WeakRefTest04
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 * -#step1: 声明一个ReferenceQueue类的实例对象srq，以stringBuffer1和srq为参数，创建一个WeakReference类的实例对象并赋值给sr，
 *          判断sr.get()的返回值不为null；
 * -#step2: 重复step1四次后，令stringBuffer1等于null；
 * -#step3: gc后，经判断sr不为null；
 * -#step4: 判断得出sr.get()的返回值为null；
 * -#step5: 重复步骤4四次后，经判断srq.poll()的返回值不为null，并且其是WeakReference类的一个实例对象；
 * -#step6: 创建一个StringBuffer类型的变量stringBuffer2，并赋初值为weak，同时声明一个ReferenceQueue类的实例对象wrq，以
 *          stringBuffer2和wrq为参数，创建一个WeakReference类的实例对象并赋值给wr，经判断wr.get()的返回值不为null；
 * -#step7: 重复step6三次后，令stringBuffer2等于null；让当前线程休眠2000ms，经判断wr不为null；
 * -#step8: 判断得出wr.get()的返回值不为null，否则将check_count的值加1，并打印相关信息；
 * -#step9: 重复步骤8三次后，经判断wrq.poll()的返回值不为null，并且其是WeakReference类的一个实例对象；
 * -#step10: 创建一个StringBuffer类型的变量stringBuffer3，并赋初值为phantom，同时声明一个ReferenceQueue类的实例对象prq，
 *           以stringBuffer3和prq为参数，创建一个WeakReference类的实例对象并赋值给pr，经判断pr.get()的返回值不为null；
 * -#step11: 重复step10四次后，令stringBuffer3等于null；
 * -#step12: 经判断prq.poll()的返回值不为null，并且其是WeakReference类的一个实例对象；
 * -#step13: 重复步骤1~12，但是在执行步骤7的时候，由原来的重复步骤step6三次变为四次；同样，在执行步骤9的时候，由原来的
 *           重复步骤8三次变为四次；
 * -#step14: 重复步骤1~12，但是在执行步骤7的时候，由原来的重复步骤step6三次变为五次；同样，在执行步骤9的时候，由原来的
 *           重复步骤8三次变为五次；
 * -#step15: 让当前线程休眠1000ms；
 * -#step16: 调用Runtime.getRuntime().gc()进行垃圾回收；
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest04.java
 *- @ExecuteClass: GCOnly_WeakRefTest04
 *- @ExecuteArgs:
 *- @Remark:RC0342-rc-rp-weak_ref_boundary
 *- @Author:zhangwenlong 00292413
 */

import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class GCOnly_WeakRefTest04 {
    static final int TEST_NUM = 1;
    static int checkCount = 0;
    static WeakReference[] wr = new WeakReference[500];
    static WeakReference[] sr = new WeakReference[500];
    static PhantomReference[] pr = new PhantomReference[500];
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static StringBuffer stringBuffer1 = new StringBuffer("soft");
    static StringBuffer stringBuffer2 = new StringBuffer("weak");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");

    public static void main(String[] args) {
        test();
        if (checkCount == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("errorResult,checkNum: " + checkCount);
        }
    }

    private static void test() {
        //test01: 14 mix weak references
        for (int i = 0; i < TEST_NUM; i++) {
            test01();
        }
        sleep(1000);
    }

    public static void setSoftRef(int times) {
        stringBuffer1 = new StringBuffer("soft");
        for (int i = 0; i < times; i++) {
            sr[i] = new WeakReference<Object>(stringBuffer1, srq);
            if (sr[i].get() == null) {
                System.out.println(" ---------------error in sr of test01");
                checkCount++;
            }
        }
        stringBuffer1 = null;
    }

    public static void setWeakRef(int times) {
        stringBuffer2 = new StringBuffer("Weak");
        for (int i = 0; i < times; i++) {
            wr[i] = new WeakReference<Object>(stringBuffer2, wrq);
            if (wr[i].get() == null) {
                System.out.println(" ---------------error in wr of test01");
                checkCount++;
            }
        }
        stringBuffer2 = null;
    }

    public static void setPhantomRef(int times) {
        stringBuffer3 = new StringBuffer("phantom");
        for (int i = 0; i < times; i++) {
            pr[i] = new PhantomReference<Object>(stringBuffer3, prq);
            if (pr[i].get() != null) {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ---------------" +
                        "error in pr of test01");
                checkCount++;
            }
        }
        stringBuffer3 = null;
    }

    public static void test01() {
        /* 14 mix weak references */
        setSoftRef(500);
        checkSoftRq("soft", 500);
        setWeakRef(500);
        checkWeakRq("weak", 500);
        setPhantomRef(500);
        checkPhantomRq("phantom");
    }

    public static void checkSoftRq(String funName, int times) {
        Reference srqPoll;
        Runtime.getRuntime().gc();
        if (sr != null) {
            for (int i = 0; i < times; i++) {
                if (sr[i].get() != null) {
                    checkCount++;
                    System.out.println(" ErrorResult in sr      " + funName);
                }
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in srq.poll()");
            }
        }
    }

    public static void checkWeakRq(String funName, int times) {
        Reference wrqPoll;
        Runtime.getRuntime().gc();
        for (int i = 0; i < times; i++) {
            if (wr[i].get() != null) {
                checkCount++;
                System.out.println(" ErrorResult in wr       " + funName + "times:" + i);
            }
        }
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in wrq.poll()");
            }
        }
    }

    public static void checkPhantomRq(String funName) {
        Reference prqPoll;
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


