/*
 *- @TestCaseID: RefProCase/RefProcessor/src/IsCleanerNotInDeadCycleNotFreeRefSetWCB.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: RefProcessor basic testcase：构建可达的环，为cleaner对象，不释放对象，设置WCB
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建一个Cycle_BDec_00010_A1_Cleaner类的实例对象cycleBDA1Cleaner，以cycleBDA1Cleaner为参数，执行
 *          resultSetCycle()方法 ，即创建一个InCycle类的实例对象cycleA，以Cycle_BDec_00010_A1_Cleaner类的实例对象
 *          cycleBDA为参数，执行cycleA的setCleanerCycle()方法，得到返回值result；
 * -#step2: 调用Runtime.getRuntime().gc()执行垃圾回收；
 * -#step3: 令cycleBDA1Cleaner.cycleBDA2Cleaner的值为null；
 * -#step4: 经判断得知result为true，并且cycleBDA1Cleaner在执行clearSoftReference()方法时返回为false，得知
 *          cycleBDA1Cleaner在清除软引用时失败；
 * -#step5: 调用Runtime.getRuntime().gc()进行垃圾回收；
 * -#step6: 重复步骤1~4；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: IsCleanerNotInDeadCycleNotFreeRefSetWCB.java
 *- @ExecuteClass: IsCleanerNotInDeadCycleNotFreeRefSetWCB
 *- @ExecuteArgs:
 *- @Remark:
 */

import sun.misc.Cleaner;

import java.lang.ref.*;

public class IsCleanerNotInDeadCycleNotFreeRefSetWCB {
    static int TEST_NUM = 1;
    static int judgeNum = 0;

    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            isCleanerNotInDeadCycleNotFreeRefSetWCB();
            Runtime.getRuntime().gc();
            isCleanerNotInDeadCycleNotFreeRefSetWCB();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }

    static boolean resultSetCycle(Cycle_BDec_00010_A1_Cleaner cycleBDA) {
        InCycle cycleA = new InCycle();
        boolean result = cycleA.setCleanerCycle(cycleBDA);
        Runtime.getRuntime().gc();
        return result;
    }

    static void isCleanerNotInDeadCycleNotFreeRefSetWCB() throws InterruptedException {
        Cycle_BDec_00010_A1_Cleaner cycleBDA1Cleaner = new Cycle_BDec_00010_A1_Cleaner();
        boolean result = resultSetCycle(cycleBDA1Cleaner);
        cycleBDA1Cleaner.cycleBDA2Cleaner = null;

        if (result == true) {
            if (cycleBDA1Cleaner.clearSoftReference() == true) {
                if (cycleBDA1Cleaner.clearWeakReference() == true) {
                    if (cycleBDA1Cleaner.clearFinalizePhantomReference() == true) {
                        if (cycleBDA1Cleaner.clearNoFinalizePhantomReference() == true) {
                            if (cycleBDA1Cleaner.clearFinalizeCleaner() == true) {
                                if (cycleBDA1Cleaner.clearNoFinalizeCleaner() == true) {
                                } else {
                                    judgeNum++;
                                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                            + " ErrorResult in clearNoFinalizeCleaner------------" +
                                            "isCleanerNotInDeadCycleNotFreeRefSetWCB");
                                }
                            } else {
                                judgeNum++;
                                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                        + " ErrorResult in clearFinalizeCleaner------------" +
                                        "isCleanerNotInDeadCycleNotFreeRefSetWCB");
                            }
                        } else {
                            judgeNum++;
                            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                    + " ErrorResult in clearNoFinalizePhantomReference------------" +
                                    "isCleanerNotInDeadCycleNotFreeRefSetWCB");
                        }
                    } else {
                        judgeNum++;
                        System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                                + " ErrorResult in clearFinalizePhantomReference------------" +
                                "isCleanerNotInDeadCycleNotFreeRefSetWCB");
                    }
                } else {
                    judgeNum++;
                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                            + " ErrorResult in clearWeakReference------------isCleanerNotInDeadCycleNotFreeRefSetWCB");
                }
            } else {
                judgeNum++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                        + " ErrorResult in clearSoftReference------------isCleanerNotInDeadCycleNotFreeRefSetWCB");
            }
        } else {
            judgeNum++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                    + " ErrorResult in isCleanerNotInDeadCycleNotFreeRefSetWCB------------" +
                    "isCleanerNotInDeadCycleNotFreeRefSetWCB");
        }
    }
}

class Cycle_BDec_00010_A2_Cleaner {
    Cleaner cleaner;
    Cycle_BDec_00010_A1_Cleaner cycleBDA1;
    int num;
    int sum;
    static int value;

    Cycle_BDec_00010_A2_Cleaner() {
        cleaner.create(cycleBDA1, null);
        cycleBDA1 = null;
        num = 2;
        sum = 0;
        value = 100;
    }

    void add() {
        sum = num + cycleBDA1.num;
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
    static StringBuffer obj = new StringBuffer("soft");
    static StringBuffer obj1 = new StringBuffer("weak");
    static StringBuffer obj2 = new StringBuffer("phantom");

    static void setSoftReference() {
        obj = new StringBuffer("soft");
        srp = new WeakReference<Object>(obj, srq);
        if (srp.get() == null) {
            value++;
        }
        obj = null;
    }

    static boolean clearSoftReference() throws InterruptedException {
        int value = 100;
        Reference r;
        setSoftReference();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        // 释放，应该为空
        if (srp.get() != null) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                    + " --------------------srp.get():" + srp.get());
            value++;
        }
        while ((r = srq.poll()) != null) {
            if (!r.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName()
                        + " --------------------srp.while");
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
     * 设置弱引用,clear and enqueue
     */
    static void setWeakReference() {
        obj1 = new StringBuffer("weak");
        wrp = new WeakReference<Object>(obj1, wrq);
        if (wrp.get() == null) {
            value++;
        }
        obj1 = null;
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
        prp = new PhantomReference<Object>(obj2, prq);
        if (prp.get() != null) {
            value++;
        }
        obj2 = null;
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

    /**
     * 设置虚引用, 经过Referent finalized，clear and enqueue
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
     * 设置cleaner, 没有Referent finalized,clear and enqueue
     */
    static void setCleaner() {
        Cleaner crp = null;
        StringBuffer str = new StringBuffer("test");
        crp.create(str, null);
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

    static int finalSet() {
        RC_Finalize testClass1 = new RC_Finalize();
        System.runFinalization();
        return testClass1.value;
    }

    /**
     * 设置cleaner, 经过Referent finalized，clear and enqueue
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
        // 初次调用， Cleaner即为空
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
        if (nSum == 6) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 设置一个带Cleaner的环
     *
     * @param cycleBD 传入的是带有Referent的类实例
     * @return true:正确；false：错误
     */
    public static boolean setCleanerCycle(Cycle_BDec_00010_A1_Cleaner cycleBD) {
        cycleBD.cycleBDA2Cleaner = new Cycle_BDec_00010_A2_Cleaner();
        cycleBD.cycleBDA2Cleaner.cycleBDA1 = cycleBD;
        boolean ret;
        ret = ModifyCleanerA1(cycleBD);
        //环正确，且reference都没释放
        if (ret == true && cycleBD.cleaner == null && cycleBD.cycleBDA2Cleaner.cleaner == null
                && cycleBD != null && cycleBD.cycleBDA2Cleaner != null) {
            return true;
        } else {
            return false;
        }
    }
}

class ThreadRef implements Runnable {
    public void run() {
        for (int i = 0; i < 60; i++) {
            try {
                Thread.sleep(100);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}

class RC_Finalize {
    static int value = 0;

    protected void finalize() throws Throwable {
        super.finalize();
        value = 200;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
