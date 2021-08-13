/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest02
 *- @TestCaseName: GCOnly_WeakRefTest02
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 无
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 * -#step1: 分别创建CycleA、CycleB、CycleC、CycleD 四个类的对象，均含有弱引用，并且这四个对象互为循环引用；
 * -#step2: 分别取出step1中四个对象的sum属性的值，计算它们的和并赋值给allSum；
 * -#step3: 触发GC；
 * -#step4: 确认四个类的对象的引用均已经释放；
 * -#step5: 将step1中CycleA的实例对象cycleA赋值为null；
 * -#step6: 让当前线程休眠2000ms；
 * -#step7: 经判断得出step1中的四个实例对象均为null；
 * -#step8: 分别判断allSum和checkCount的值，并确定step1中四个互为循环引用的对象为弱引用；
 * -#step9: 分别判断allSum和checkCount的值，并确定step1中四个互为循环引用的对象为弱引用；
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest02.java
 *- @ExecuteClass: GCOnly_WeakRefTest02
 *- @ExecuteArgs:
 *- @Remark:RC0339-rc-rp-CycleAll_weak
 *- @Author:zhangwenlong 00292413
 */

import java.lang.ref.Reference;
import java.lang.ref.WeakReference;

public class GCOnly_WeakRefTest02 {
    static int checkCount = 0;
    static final int TEST_NUM = 1;
    static int allSum;

    public static void main(String[] args) {
        checkCount = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            test01();
            if (allSum == 20) {
                checkCount++;
                System.out.println("sum is wrong");
            }

            if (checkCount == 0) {
                System.out.println("ExpectResult");
            } else {
                System.out.println("errorResult ---- checkCount: " + checkCount);
            }
        }
    }

    private static void test01() {
        CycleA cycleA = new CycleA();
        cycleA.cyb = new CycleB();
        cycleA.cyb.cyc = new CycleC();
        cycleA.cyb.cyc.cyd = new CycleD();
        cycleA.cyb.cyc.cyd.cya = cycleA;
        allSum = cycleA.sum + cycleA.cyb.sum + cycleA.cyb.cyc.sum + cycleA.cyb.cyc.cyd.sum;
        Runtime.getRuntime().gc();
        if ((cycleA.wr.get() != null) || (cycleA.cyb.sr.get() != null) || (cycleA.cyb.cyc.pr.get() != null)
                || (cycleA.cyb.cyc.cyd.str.get() != null)) {
            checkCount++;
        }
        cycleA = null;
        sleep(2000);
        if (cycleA != null && cycleA.cyb != null && cycleA.cyb.cyc != null && cycleA.cyb.cyc.cyd != null
                && cycleA.cyb.cyc.cyd.cya != null) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " [test01]a1 has not free");
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

class CycleA {
    Reference wr;
    CycleB cyb;
    int aNum;
    int sum;
    static StringBuffer stringBufferA = new StringBuffer("ref_processor_CycleA");

    CycleA() {
        stringBufferA = new StringBuffer("ref_processor_CycleA");
        wr = new WeakReference<>(stringBufferA);
        if (wr.get() == null) {
            assert false;
        }
        stringBufferA = null;
        cyb = null;
        aNum = 1;
    }

    int joinStr() {
        try {
            sum = aNum + cyb.bNum;
        } catch (NullPointerException e) {
            // do nothing
        }
        return sum;
    }
}

class CycleB {
    Reference sr;
    CycleC cyc;
    int bNum;
    int sum;
    static StringBuffer stringBufferB = new StringBuffer("ref_processor_CycleB");

    CycleB() {
        stringBufferB = new StringBuffer("ref_processor_CycleB");
        sr = new WeakReference<>(stringBufferB);
        if (sr.get() == null) {
            assert false;
        }
        stringBufferB = null;
        cyc = null;
        bNum = 2;
    }

    int joinStr() {
        try {
            sum = bNum + cyc.cNum;
        } catch (NullPointerException e) {
            // do nothing
        }
        return sum;
    }
}

class CycleC {
    Reference pr;
    CycleD cyd;
    StringBuffer stringBufferC = new StringBuffer("ref_processor_CycleC");
    int sum;
    int cNum;

    CycleC() {
        stringBufferC = new StringBuffer("ref_processor_CycleC");
        pr = new WeakReference<>(stringBufferC);
        if (pr.get() == null) {
            assert false;
        }
        stringBufferC = null;
        cyd = null;
        cNum = 3;
    }

    int joinStr() {
        try {
            sum = cNum + cyd.dNum;
        } catch (NullPointerException e) {
            // do nothing
        }
        return sum;
    }
}

class CycleD {
    Reference str;
    CycleA cya;
    StringBuffer stringBufferD = new StringBuffer("ref_processor_CycleD");
    int dNum;
    int sum;

    CycleD() {
        stringBufferD = new StringBuffer("ref_processor_CycleD");
        str = new WeakReference<StringBuffer>(stringBufferD);
        stringBufferD = null;
        cya = null;
        dNum = 4;
    }

    int joinStr() {
        try {
            sum = dNum + cya.aNum;
        } catch (NullPointerException e) {
            // do nothing
        }
        return sum;
    }
}

