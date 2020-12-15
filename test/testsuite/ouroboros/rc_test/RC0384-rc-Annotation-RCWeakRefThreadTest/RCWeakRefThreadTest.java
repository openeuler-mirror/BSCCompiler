/*
 *- @TestCaseID:maple/runtime/rc/function/RC_Thread01/RCWeakRefThreadTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Change RCWeakRefTest4 to Multi thread testcase.
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCWeakRefThreadTest.java
 *- @ExecuteClass: RCWeakRefThreadTest
 *- @ExecuteArgs:
 *- @Remark:
 */

import com.huawei.ark.annotation.Weak;

class ThreadRc_Cycle_Bm_1_00010 extends Thread {
    private boolean checkout;

    public void run() {
        Cycle_B_1_00010_A1 a1_0 = new Cycle_B_1_00010_A1();
        a1_0.a1_0 = a1_0;
        a1_0.add();
        int nsum = a1_0.sum;
        if (nsum == 246)
            checkout = true;
    }

    public boolean check() {
        return checkout;
    }

    class Cycle_B_1_00010_A1 {
        @Weak
        Cycle_B_1_00010_A1 a1_0;
        int a;
        int sum;

        Cycle_B_1_00010_A1() {
            a1_0 = null;
            a = 123;
            sum = 0;
        }

        void add() {
            sum = a1_0.a + this.a;
        }
    }
}

public class RCWeakRefThreadTest {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
    }

    private static void rc_testcase_main_wrapper() {
        ThreadRc_Cycle_Bm_1_00010 A1_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A2_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A3_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A4_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A5_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A6_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        A1_Cycle_Bm_1_00010.start();
        A2_Cycle_Bm_1_00010.start();
        A3_Cycle_Bm_1_00010.start();
        A4_Cycle_Bm_1_00010.start();
        A5_Cycle_Bm_1_00010.start();
        A6_Cycle_Bm_1_00010.start();
        try {
            A1_Cycle_Bm_1_00010.join();
            A2_Cycle_Bm_1_00010.join();
            A3_Cycle_Bm_1_00010.join();
            A4_Cycle_Bm_1_00010.join();
            A5_Cycle_Bm_1_00010.join();
            A6_Cycle_Bm_1_00010.join();
        } catch (InterruptedException e) {
            // do nothing
        }
        if (A1_Cycle_Bm_1_00010.check() && A2_Cycle_Bm_1_00010.check() && A3_Cycle_Bm_1_00010.check() && A4_Cycle_Bm_1_00010.check() && A5_Cycle_Bm_1_00010.check() && A6_Cycle_Bm_1_00010.check()) {
            System.out.println("ExpectResult");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
