/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.io.PrintStream;
import java.lang.ref.*;
public class NotIsCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            notIsCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB();
            Runtime.getRuntime().gc();
            notIsCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }
    static boolean resultSetCycle(Cycle_BDec_00010_A1 cycle1) throws InterruptedException {
        InCycle cycleA = new InCycle();
        cycleA.setCycle(cycle1);
        Runtime.getRuntime().gc();
        boolean result = cycleA.setCycle(cycle1);
        return result;
    }
    static void notIsCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB() throws InterruptedException {
        Cycle_BDec_00010_A1 cycle1 = new Cycle_BDec_00010_A1();
        boolean result = resultSetCycle(cycle1);
        if (result == true) {
            cycle1.partner2 = null;
            cycle1 = null;
            if (cycle1.clearSoftReference() == true) {
                if (cycle1.clearWeakReference() == true) {
                    if (cycle1.clearFinalizePhantomReference() == true) {
                        if (cycle1.clearNoFinalizePhantomReference() == true) {
                        } else {
                            judgeNum++;
                            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                                    " ErrorResult in clearNoFinalizePhantomReference--------------" +
                                    "notIsCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                        }
                    } else {
                        judgeNum++;
                        System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                                " ErrorResult in clearFinalizePhantomReference-------------" +
                                "notIsCleanerInDeadCycleNotFreeRefFailSetWCBFailAtomicWCB");
                    }
                } else {
                    judgeNum++;
                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                            " ErrorResult in clearWeakReference");
                }
            } else {
                judgeNum++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                        " ErrorResult in clearSoftReference");
            }
        } else {
            judgeNum++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                    " ErrorResult in isCleanerNotInDeadCycleNotFreeRefSetWCB");
        }
    }
}
class Cycle_BDec_00010_A1 {
    Reference a1Ref;
    static ReferenceQueue rq = new ReferenceQueue();
    Cycle_BDec_00010_A2 partner2;
    int num;
    int sum;
    static int value;
    static StringBuffer obj = new StringBuffer("weak");
    Cycle_BDec_00010_A1() {
        obj = new StringBuffer("weak");
        a1Ref = new WeakReference<Object>(obj, rq);
        if (a1Ref.get() == null) {
            assert false;
        }
        obj = null;
        partner2 = null;
        num = 1;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + partner2.num;
    }
    /**
     * 设置软引用,clear and enqueue
   */


    static Reference srp, wrp, prp;
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static StringBuffer obj1 = new StringBuffer("soft");
    static StringBuffer obj2 = new StringBuffer("weak");
    static StringBuffer obj3 = new StringBuffer("phantom");
    static void setSoftReference() {
        obj1 = new StringBuffer("soft");
        srp = new WeakReference<Object>(obj1, srq);
        if (srp.get() == null) {
            value++;
        }
        obj1 = null;
    }
    static boolean clearSoftReference() throws InterruptedException {
        int value = 100;
        Reference r;
        setSoftReference();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        //释放，应该为空
        if (srp.get() != null) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                    " --------------------srp.get():" + srp.get());
            value++;
        }
        while ((r = srq.poll()) != null) {
            if (!r.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                        " --------------------srp.while");
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
        obj2 = new StringBuffer("weak");
        wrp = new WeakReference<Object>(obj2, wrq);
        if (wrp.get() == null) {
            value++;
        }
        obj2 = null;
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
     * 设置虚引用, 没有Referent finalized, clear and enqueue
   */


    static void setPhantomReference() {
        obj3 = new StringBuffer("phantom");
        prp = new PhantomReference<Object>(obj3, prq);
        if (prp.get() != null) {
            value++;
        }
        obj3 = null;
    }
    static boolean clearNoFinalizePhantomReference() throws InterruptedException {
        value = 100;
        Reference reference;
        setPhantomReference();
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
}
class Cycle_BDec_00010_A2 {
    Reference a2Ref;
    static ReferenceQueue rq = new ReferenceQueue();
    Cycle_BDec_00010_A1 partner1;
    int num;
    int sum;
    static int value;
    static StringBuffer obj = new StringBuffer("weak");
    Cycle_BDec_00010_A2() {
        obj = new StringBuffer("weak");
        a2Ref = new WeakReference<Object>(obj, rq);
        if (a2Ref.get() == null) {
            assert false;
        }
        obj = null;
        partner1 = null;
        num = 2;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + partner1.num;
    }
}
class InCycle {
    /**
     * 确认环是正确的
     *
     * @param refInstance 传入的是带有Referent的类实例
     * @return true: 正确；false：错误
   */


    public static boolean ModifyA1(Cycle_BDec_00010_A1 refInstance) {
        refInstance.add();
        refInstance.partner2.add();
        int nSum = refInstance.sum + refInstance.partner2.sum;
        if (nSum == 6) {
            return true;
        } else {
            return false;
        }
    }
    /**
     * 设置一个带Referent的环
     *
     * @param refInstance 传入的是带有Referent的类实例
     * @return true: 正确；false：错误
   */


    public static boolean setCycle(Cycle_BDec_00010_A1 refInstance) throws InterruptedException {
        refInstance.partner2 = new Cycle_BDec_00010_A2();
        if (refInstance.partner2.a2Ref.get() == null) {
            return false;
        }
        refInstance.partner2.partner1 = refInstance;
        boolean ret;
        ret = ModifyA1(refInstance);
        if (ret == true) {
            Thread.sleep(2000);
            if (refInstance.a1Ref.get() == null) {
                if (refInstance != null && refInstance.partner2 != null) {
                    return true;
                } else {
                    System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                            " a1_0==null||a1_0.a2_0==nulll");
                    return false;
                }
            } else {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() +
                        " a1_0.a1Ref.get() != null");
                return false;
            }
        } else {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ret != true");
            return false;
        }
    }
}
class RC_Finalize {
    static int value = 0;
    static void rc_Finalize() throws InterruptedException {
        RC_Finalize rc = new RC_Finalize();
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
            } catch (InterruptedException e) {
                System.out.println(" sleep was Interrupted");
            }
        }
    }
}