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
