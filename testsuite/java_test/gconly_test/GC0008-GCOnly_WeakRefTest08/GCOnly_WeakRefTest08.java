/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest08
 *- @TestCaseName: GCOnly_WeakRefTest08
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 * -#step1: 分别创建Cycle_BDec_00010_A1_Cleaner类、InCycle类的实例对象cycleBDA1Cleaner、cycleA；
 * -#step2: 以cycleBDA1为参数，执行cycleA的setCleanerCycle()方法，并记返回值为result；
 * -#step3: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step4: 经判断得知result为true，并对cycleBDA1Cleaner对象清除软引用、弱引用、NoFinalizePhantomReference、
 *          FinalizePhantomReference、FinalizeCleaner、NoFinalizeCleaner，并且都返回true，证明都清除成功；
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest08.java
 *- @ExecuteClass: GCOnly_WeakRefTest08
 *- @ExecuteArgs:
 *- @Remark:RC0350-rc-rp-simpleweakref_03
 *- @Author:
 */

import sun.misc.Cleaner;
import java.util.ArrayList;
import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class GCOnly_WeakRefTest08 {
    static int testNum = 1;
    static int judgeNum = 0;

    public static void main(String[] args) throws Throwable {
        judgeNum = 0;
        for (int i = 0; i < testNum; i++) {
            isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error,judgeNum:" + judgeNum);
        }
    }

    static boolean resultSetCycle(Cycle_BDec_00010_A1_Cleaner cycleBDA1) {
        InCycle cycleA = new InCycle();
        boolean result = InCycle.setCleanerCycle(cycleBDA1);
        Runtime.getRuntime().gc();
        return result;
    }

    static void isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB() throws Throwable {
        Cycle_BDec_00010_A1_Cleaner cycleBDA1Cleaner = new Cycle_BDec_00010_A1_Cleaner();
        boolean result = resultSetCycle(cycleBDA1Cleaner);
        if (result == true) {
            cycleBDA1Cleaner.cycleBDA2Cleaner = null;
            cycleBDA1Cleaner = null;
            if (result == true) {
                if (Cycle_BDec_00010_A1_Cleaner.clearSoftReference() == true) {
                    if (Cycle_BDec_00010_A1_Cleaner.clearWeakReference() == true) {
                        if (Cycle_BDec_00010_A1_Cleaner.clearFinalizePhantomReference() == true) {
                            if (Cycle_BDec_00010_A1_Cleaner.clearNoFinalizePhantomReference() == true) {
                                if (Cycle_BDec_00010_A1_Cleaner.clearFinalizeCleaner() == true) {
                                    if (Cycle_BDec_00010_A1_Cleaner.clearNoFinalizeCleaner() == true) {
                                    } else {
                                        judgeNum++;
                                        System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                            + " ErrorResult in clearNoFinalizeCleaner--------------"
                                            + "isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                                    }
                                } else {
                                    judgeNum++;
                                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                        + " ErrorResult in clearFinalizeCleaner------------------"
                                        + "isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                                }
                            } else {
                                judgeNum++;
                                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                    + " ErrorResult in clearNoFinalizePhantomReference---------------"
                                    + "isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                            }
                        } else {
                            judgeNum++;
                            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                + " ErrorResult in clearFinalizePhantomReference-----------------------"
                                + "isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                        }
                    } else {
                        judgeNum++;
                        System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult "
                            + "in clearWeakReference------------------"
                            + "isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                    }
                } else {
                    judgeNum++;
                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in "
                        + "clearSoftReference-----------------isCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                }
            } else {
                judgeNum++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in "
                    + "isCleanerNotInDeadCycleNotFreeRefSetWCB");
            }
        }
    }
}

class Cycle_BDec_00010_A1_Cleaner {
    static Cleaner cleaner;
    static ReferenceQueue rq = new ReferenceQueue();
    static int value;
    /**
     * 设置软引用,clear and enqueue
     */
    static Reference srp, wrp, prp;
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static ReferenceQueue crq = new ReferenceQueue();
    static StringBuffer stringBuffer1 = new StringBuffer("soft");
    static StringBuffer stringBuffer2 = new StringBuffer("weak");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");
    Cycle_BDec_00010_A2_Cleaner cycleBDA2Cleaner;
    int num;
    int sum;
    Cycle_BDec_00010_A1_Cleaner() {
        Cleaner.create(cycleBDA2Cleaner, null);
        cycleBDA2Cleaner = null;
        num = 1;
        sum = 0;
        value = 100;
    }

    static void setSoftReference() {
        stringBuffer1 = new StringBuffer("soft");
        srp = new WeakReference<Object>(stringBuffer1, srq);
        if (srp.get() == null) {
            value++;
        }
        stringBuffer1 = null;
    }

    private static int oomTest() {
        int sum = 0;
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;

        for (int i = 1044 * 1044; i <= 1044 * 1044 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }

    static boolean clearSoftReference() throws InterruptedException {
        int value = 100;
        Reference r;
        setSoftReference();
        new Thread(new ThreadRf()).start();
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            // do nothing
        }
        Thread.sleep(2000);
        // 释放，应该为空
        if (srp.get() != null) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " --------------------"
                + "srp.get():" + srp.get());
            value++;
        }
        while ((r = srq.poll()) != null) {
            if (!r.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " --------------------"
                    + "srp.while");
                value++;
            }
        }
        return value == 100;
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
        new Thread(new ThreadRf()).start();
        Runtime.getRuntime().gc();
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
        return value == 100;
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
        new Thread(new ThreadRf()).start();
        Thread.sleep(2000);
        while ((reference = prq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                value++;
            }
        }
        return value == 100;
    }

    /**
     * 设置虚引用, 经过Referent finalized，clear and enqueue
     */
    static boolean clearFinalizePhantomReference() throws Throwable {
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
        return value == 200;
    }

    /**
     * 设置cleaner, 没有Referent finalized,clear and enqueue
     */
    static void setCleaner() {
        Cleaner crp = null;
        String str = "test";
        Cleaner.create(str, null);
        try {
            if (crp.get() != null) {
                value++;
            }
        } catch (NullPointerException e) {
        }
    }

    static boolean clearNoFinalizeCleaner() throws InterruptedException {
        Reference reference;
        value = 100;
        setCleaner();
        new Thread(new ThreadRf()).start();
        Thread.sleep(2000);
        while ((reference = crq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.Cleaner")) {
                value++;
            }
        }
        return value == 100;
    }

    /**
     * 设置cleaner, 经过Referent finalized，clear and enqueue
     */
    static int finalSet() {
        RC_Finalize testClass1 = new RC_Finalize();
        System.runFinalization();
        return RC_Finalize.value;
    }

    static boolean clearFinalizeCleaner() throws InterruptedException {
        Reference reference;
        value = 100;
        int finalValue = finalSet();
        Runtime.getRuntime().gc();
        if (finalValue == 200) {
            value = value + 100;
        }
        setCleaner();
        // 初次调用， Cleaner即为空
        new Thread(new ThreadRf()).start();
        Thread.sleep(2000);
        while ((reference = rq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.Cleaner")) {
                value++;
            }
        }
        return value == 200;
    }

    void add() {
        sum = num + cycleBDA2Cleaner.num;
    }
}

class InCycle {
    /**
     * 确认环是正确的
     *
     * @param cycleB 传入的是带有Referent的类实例
     * @return true:正确；false：错误
     */
    public static boolean ModifyCleanerA1(Cycle_BDec_00010_A1_Cleaner cycleB) {
        cycleB.add();
        cycleB.cycleBDA2Cleaner.add();
        int nSum = cycleB.sum + cycleB.cycleBDA2Cleaner.sum;
        return nSum == 6;
    }

    /**
     * 设置一个带Cleaner的环
     *
     * @param cycleBDA1Cleaner 传入的是带有Referent的类实例
     * @return true:正确；false：错误
     */
    public static boolean setCleanerCycle(Cycle_BDec_00010_A1_Cleaner cycleBDA1Cleaner) {
        cycleBDA1Cleaner.cycleBDA2Cleaner = new Cycle_BDec_00010_A2_Cleaner();
        cycleBDA1Cleaner.cycleBDA2Cleaner.cycleBD = cycleBDA1Cleaner;
        boolean ret;
        ret = ModifyCleanerA1(cycleBDA1Cleaner);
        // 环正确，且reference都没释放
        return ret == true && Cycle_BDec_00010_A1_Cleaner.cleaner == null
            && cycleBDA1Cleaner.cycleBDA2Cleaner.cleaner == null
            && cycleBDA1Cleaner != null && cycleBDA1Cleaner.cycleBDA2Cleaner != null;
    }
}

class Cycle_BDec_00010_A2_Cleaner {
    static int value;
    Cleaner cleaner;
    Cycle_BDec_00010_A1_Cleaner cycleBD;
    int num;
    int sum;

    Cycle_BDec_00010_A2_Cleaner() {
        Cleaner.create(cycleBD, null);
        cycleBD = null;
        num = 2;
        sum = 0;
        value = 100;
    }

    void add() {
        sum = num + cycleBD.num;
    }
}

class RC_Finalize {
    static int value = 0;

    protected void finalize() throws Throwable {
        super.finalize();
        value = 200;
    }
}

class ThreadRf implements Runnable {
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
