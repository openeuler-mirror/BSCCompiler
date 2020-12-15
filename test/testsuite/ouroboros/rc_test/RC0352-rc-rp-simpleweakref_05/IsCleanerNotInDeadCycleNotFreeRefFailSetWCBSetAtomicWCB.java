/*
 *- @TestCaseID: RefProCase/RefProcessor/src/IsCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: RefProcessor basic testcase: 不是死对象，是cleaner对象，在可达的环中，referent没有释放，
 *                      没有设置WCB，atomic设置WCB，线程处理，一个线程负责Referent的释放，另外一个负责相关设置
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建一个SetReferentNull类的实例对象setReferentNull，分别创建Cycle_BDec_00010_A1_Cleaner类、
 *          Cycle_BDec_00010_A2_Cleaner类的实例对象a1_cleaner、a2_cleaner，并设初值为null；
 * -#step2: 创建一个IsCleanerNotInDeadCycleNotFreeRefFailSetWCB类的实例对象isCleanerNotInDeadCycleNotFreeRefFailSetWCB，
 *          并令其执行所属类的run()方法；
 * -#step3: 创建Cycle_BDec_00010_A1_Cleaner类的实例对象cycleB，以cycleB为参数，执行resultSetCycle()方法，即创建一个
 *          InCycle类的实例对象cycleA，以cycleBDA为参数，执行cycleA的setCleanerCycle()方法，并得其返回值为result；
 * -#step4: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step5: 经判断得知result为true，并且cycleB在执行clearSoftReference()方法时返回false，并将返回值赋值给check，由此得知
 *          此处清除软引用失败；
 * -#step6: 因check的值为false，则将judgeNum的值加1；
 * -#step7: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step8: 重复步骤1~6；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: IsCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB.java
 *- @ExecuteClass: IsCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB
 *- @ExecuteArgs:
 *- @Remark:
 */

import sun.misc.Cleaner;

import java.io.PrintStream;
import java.lang.ref.*;

public class IsCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB {
    static int TEST_NUM = 1;
    static int judgeNum = 0;

    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            isCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB();
            Runtime.getRuntime().gc();
            isCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }

    static void isCleanerNotInDeadCycleNotFreeRefFailSetWCBSetAtomicWCB() throws Exception {
        SetReferentNull setReferentNull = new SetReferentNull();
        setReferentNull.run();
        IsCleanerNotInDeadCycleNotFreeRefFailSetWCB isCleanerNotInDeadCycleNotFreeRefFailSetWCB =
                new IsCleanerNotInDeadCycleNotFreeRefFailSetWCB();
        isCleanerNotInDeadCycleNotFreeRefFailSetWCB.run();
        if (isCleanerNotInDeadCycleNotFreeRefFailSetWCB.check == false) {
            judgeNum++;
        }
    }
}

class SetReferentNull implements Runnable {
    Cycle_BDec_00010_A1_Cleaner a1_cleaner;
    Cycle_BDec_00010_A2_Cleaner a2_cleaner;

    @Override
    public void run() {
        a2_cleaner = null;
        a1_cleaner = null;
    }
}

class IsCleanerNotInDeadCycleNotFreeRefFailSetWCB implements Runnable {
    boolean check;

    @Override
    public void run() {
        check = isCleanerNotInDeadCycleNotFreeRefFailSetWCB();
    }

    static boolean resultSetCycle(Cycle_BDec_00010_A1_Cleaner cycleBDA) {
        InCycle cycleA = new InCycle();
        boolean result = cycleA.setCleanerCycle(cycleBDA);
        Runtime.getRuntime().gc();
        return result;
    }

    public boolean isCleanerNotInDeadCycleNotFreeRefFailSetWCB() {
        Cycle_BDec_00010_A1_Cleaner cycleB = new Cycle_BDec_00010_A1_Cleaner();
        boolean result = resultSetCycle(cycleB);
        if (result == true) {
            try {
                if (result == true) {
                    if (cycleB.clearSoftReference() == true) {
                        if (cycleB.clearWeakReference() == true) {
                            if (cycleB.clearFinalizePhantomReference() == true) {
                                if (cycleB.clearNoFinalizePhantomReference() == true) {
                                    if (cycleB.clearFinalizeCleaner() == true) {
                                        if (cycleB.clearNoFinalizeCleaner() == true) {
                                            return true;
                                        } else {
                                            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                                    + " ErrorResult in clearNoFinalizeCleaner");
                                            return false;
                                        }
                                    } else {
                                        System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                                + " ErrorResult in clearFinalizeCleaner");
                                        return false;
                                    }
                                } else {
                                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                            + " ErrorResult in clearNoFinalizePhantomReference");
                                    return false;
                                }
                            } else {
                                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                        + " ErrorResult in clearFinalizePhantomReference");
                                return false;
                            }
                        } else {
                            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                    + " ErrorResult in clearWeakReference");
                            return false;
                        }
                    } else {
                        System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                + " ErrorResult in clearSoftReference");
                        return false;
                    }
                } else {
                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                            + " ErrorResult in NotIsCleanerNotInDeadCycleNotFreeRef");
                    return false;
                }
            } catch (Exception e) {
                return false;
            }
        } else {
            return false;
        }
    }
}

class InCycle {
    /**
     * 确认环是正确的
     *
     * @param cycleBDA1Cleaner 传入的是带有Referent的类实例
     * @return true:正确；false：错误
     */
    public static boolean ModifyCleanerA1(Cycle_BDec_00010_A1_Cleaner cycleBDA1Cleaner) {
        cycleBDA1Cleaner.add();
        cycleBDA1Cleaner.cycleBDA2Cleaner.add();
        int nsum = cycleBDA1Cleaner.sum + cycleBDA1Cleaner.cycleBDA2Cleaner.sum;
        if (nsum == 6) return true;
        else return false;
    }

    /**
     * 设置一个带Cleaner的环
     *
     * @param cycleBDA1 传入的是带有Referent的类实例
     * @return true:正确；false：错误
     */
    public static boolean setCleanerCycle(Cycle_BDec_00010_A1_Cleaner cycleBDA1) {
        cycleBDA1.cycleBDA2Cleaner = new Cycle_BDec_00010_A2_Cleaner();
        cycleBDA1.cycleBDA2Cleaner.cycleBD = cycleBDA1;
        boolean ret;

        ret = ModifyCleanerA1(cycleBDA1);
        // 环正确，且reference都没释放
        if (ret == true && cycleBDA1.cleaner == null && cycleBDA1.cycleBDA2Cleaner.cleaner == null
                && cycleBDA1 != null && cycleBDA1.cycleBDA2Cleaner != null) {
            return true;
        } else {
            return false;
        }
    }
}

class Cycle_BDec_00010_A2_Cleaner {
    Cleaner cleaner;
    Cycle_BDec_00010_A1_Cleaner cycleBD;
    int num;
    int sum;
    static int value;

    Cycle_BDec_00010_A2_Cleaner() {
        cleaner.create(cycleBD, null);
        cycleBD = null;
        num = 2;
        sum = 0;
        value = 100;
    }

    void add() {
        sum = num + cycleBD.num;
    }
}

class Cycle_BDec_00010_A1_Cleaner {
    static Cleaner cleaner;
    static ReferenceQueue rq = new ReferenceQueue();
    Cycle_BDec_00010_A2_Cleaner cycleBDA2Cleaner;
    int num;
    int sum;
    static int value;

    Cycle_BDec_00010_A1_Cleaner() {
        cleaner.create(cycleBDA2Cleaner, null);
        cycleBDA2Cleaner = null;
        num = 1;
        sum = 0;
        value = 100;
    }

    void add() {
        sum = num + cycleBDA2Cleaner.num;
    }

    /**
     * 设置软引用,clear and enqueue
     */
    static Reference srp, wrp, prp, crp;
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static ReferenceQueue crq = new ReferenceQueue();
    static StringBuffer stringBuffer1 = new StringBuffer("soft");
    static StringBuffer stringBuffer2 = new StringBuffer("weak");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");

    static void setSoftReference() {
        stringBuffer1 = new StringBuffer("soft");
        srp = new WeakReference<Object>(stringBuffer1, srq);
        if (srp.get() == null) {
            value++;
        }
        stringBuffer1 = null;
    }

    static boolean clearSoftReference() throws InterruptedException {
        int value = 100;
        Reference r;
        setSoftReference();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        // 释放，应该为空
        if (srp.get() != null) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " --------------------" +
                    "srp.get():" + srp.get());
            value++;
        }
        while ((r = srq.poll()) != null) {
            if (!r.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " --------------------" +
                        "srp.while");
                value++;
            }
        }
        if (value == 100) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 设置弱引用, clear and enqueue
     */
    static void setWeakReference() {
        stringBuffer2 = new StringBuffer("weak");
        wrp = new WeakReference<Object>(stringBuffer2, wrq);
        if (wrp.get() == null) {
            value++;
        }
        stringBuffer2 = null;
    }

    static boolean clearWeakReference() throws InterruptedException {
        value = 100;
        Reference reference;
        setWeakReference();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        // 释放，应该为空
        if (wrp.get() != null) {
            value++;
        }
        while ((reference = wrq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                value++;
            }
        }
        if (value == 100) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 设置虚引用, 没有Referent finalized,clear and enqueue
     */
    static void setPhantomReference() {
        stringBuffer3 = new StringBuffer("phantom");
        prp = new PhantomReference<Object>(stringBuffer3, prq);
        if (prp.get() != null) {
            value++;
        }
        stringBuffer3 = null;
    }

    static boolean clearNoFinalizePhantomReference() throws InterruptedException {
        value = 100;
        Reference reference;
        setPhantomReference();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        while ((reference = prq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                value++;
            }
        }
        if (value == 100) {
            return true;
        } else {
            return false;
        }
    }

    static int finalSet() {
        RC_Finalize testClass1 = new RC_Finalize();
        System.runFinalization();
        return testClass1.value;
    }

    /**
     * 设置虚引用,经过Referent finalized，clear and enqueue
     */
    static boolean clearFinalizePhantomReference() throws InterruptedException {
        value = 100;
        int valueFinal = finalSet();
        Runtime.getRuntime().gc();
        valueFinal = finalSet();

        Reference reference;
        setPhantomReference();
        Thread.sleep(5000);
        if (valueFinal == 200) {
            value = value + 100;
        }
        while ((reference = prq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                value++;
            }
        }
        if (value == 200) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 设置cleaner,没有Referent finalized,clear and enqueue
     */
    static void setCleaner() {
        Cleaner crp = null;
        StringBuffer stringBuffer = new StringBuffer("test");
        crp.create(stringBuffer, null);
        try {
            if (crp.get() != null) {
                value++;
            }
        } catch (NullPointerException e) {
            e.printStackTrace();
        }
    }

    static boolean clearNoFinalizeCleaner() throws InterruptedException {
        Reference reference;
        value = 100;
        setCleaner();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        while ((reference = crq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.Cleaner")) {
                value++;
            }
        }
        if (value == 100) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 设置cleaner,经过Referent finalized，clear and enqueue
     */
    static boolean clearFinalizeCleaner() throws InterruptedException {
        Reference reference;
        value = 100;
        int finalValue = finalSet();
        Runtime.getRuntime().gc();
        if (finalValue == 200) {
            value = value + 100;
        }
        setCleaner();
        // 初次调用，Cleaner即为空
        try {
            if (crp.get() != null) {
                value++;
            }
        } catch (NullPointerException e) {
            e.printStackTrace();
        }

        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        while ((reference = rq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.Cleaner")) {
                value++;
            }
        }
        if (value == 200) {
            return true;
        } else {
            return false;
        }
    }
}

class RC_Finalize {
    static int value = 0;

    static void rcFinalize() throws InterruptedException {
        RC_Finalize rc_finalize = new RC_Finalize();
        Thread.sleep(2000);
        System.runFinalization();
    }

    protected void finalize() throws Throwable {
        super.finalize();
        value = 200;
    }

    public static int run(String argv[], PrintStream out) {
        return 0;
    }
}

class ThreadRef implements Runnable {
    public void run() {
        for (int i = 0; i < 60; i++) {
            WeakReference wr = new WeakReference(new Object());
            try {
                Thread.sleep(100);
            } catch (Exception e) {
                System.out.println(" sleep was Interrupted");
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
