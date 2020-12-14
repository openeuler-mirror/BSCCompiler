/*
 *- @TestCaseID: maple/runtime/rc/function/Ref_Processor_Cycle_Weak_Strong_Ref.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Reference Processor: 1.Circular reference, has strong and weak references
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 分别创建Cycle_A、Cycle_B、Cycle_C、Cycle_D 四个类的对象，含有强弱引用，并且这四个对象互为循环引用；
 * -#step2: 分别取出step1中四个对象的sum属性的值，计算它们的和并赋值给allSum；
 * -#step3: 让当前线程休眠2000ms；
 * -#step4: 确认四个类的对象的引用均已经释放；
 * -#step5: 将step1中Cycle_A的实例对象cycleA赋值为null；
 * -#step6: 让当前线程休眠2000ms；
 * -#step7: 经判断得出step1中的四个实例对象均为null；
 * -#step8: 调用Runtime.getRuntime().gc()进行垃圾回收，重复步骤2~8；
 * -#step9: 分别判断allSum和checkCount的值，并确定step1中四个互为循环引用的对象既有强引用，又有弱引用；
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: Ref_Processor_Cycle_Weak_Strong_Ref.java
 *- @ExecuteClass: Ref_Processor_Cycle_Weak_Strong_Ref
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.*;

public class Ref_Processor_Cycle_Weak_Strong_Ref {
    static int checkCount = 0;
    static final int TEST_NUM = 1;
    static int allSum;

    public static void main(String[] args) {
        checkCount = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
            Runtime.getRuntime().gc();
            test_01();

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

    private static void test_01() {
        Cycle_A cycleA = new Cycle_A();
        cycleA.cyb = new Cycle_B();
        cycleA.cyb.cyc = new Cycle_C();
        cycleA.cyb.cyc.cyd = new Cycle_D();
        cycleA.cyb.cyc.cyd.cya = cycleA;
        allSum = cycleA.sum + cycleA.cyb.sum + cycleA.cyb.cyc.sum + cycleA.cyb.cyc.cyd.sum;
        sleep(2000);
        if ((cycleA.wr.get() != null) || (cycleA.cyb.sr.get() != null) || (cycleA.cyb.cyc.pr.get() != null)
                || (cycleA.cyb.cyc.cyd.string == null)) {
            checkCount++;
        }
        cycleA = null;
        sleep(2000);
        if (cycleA != null && cycleA.cyb != null && cycleA.cyb.cyc != null && cycleA.cyb.cyc.cyd != null
                && cycleA.cyb.cyc.cyd.cya != null) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " [test_01]a1 has not free");
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

class Cycle_A {
    Reference wr;
    Cycle_B cyb;
    int aNum;
    int sum;
    static StringBuffer stringBufferA = new StringBuffer("ref_processor_cycle_A");

    Cycle_A() {
        stringBufferA = new StringBuffer("ref_processor_cycle_A");
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
        } catch (Exception e) {
        }
        return sum;
    }
}

class Cycle_B {
    Reference sr;
    Cycle_C cyc;
    int bNum;
    int sum;
    static StringBuffer stringBufferB = new StringBuffer("ref_processor_cycle_B");

    Cycle_B() {
        stringBufferB = new StringBuffer("ref_processor_cycle_B");
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
        } catch (Exception e) {
        }
        return sum;
    }
}

class Cycle_C {
    Reference pr;
    Cycle_D cyd;
    StringBuffer stringBufferC = new StringBuffer("ref_processor_cycle_C");
    int sum;
    int cNum;

    Cycle_C() {
        stringBufferC = new StringBuffer("ref_processor_cycle_C");
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
        } catch (Exception e) {
        }
        return sum;
    }
}

class Cycle_D {
    String string;
    Cycle_A cya;
    int dNum;
    int sum;

    Cycle_D() {
        string = new String("ref_processor_cycle_D");
        cya = null;
        dNum = 4;
    }

    int joinStr() {
        try {
            int sum = dNum + cya.aNum;
        } catch (Exception e) {
        }
        return sum;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
