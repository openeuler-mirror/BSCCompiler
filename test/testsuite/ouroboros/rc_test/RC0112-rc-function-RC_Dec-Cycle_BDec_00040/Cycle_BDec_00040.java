/*
 *- @TestCaseID:maple/runtime/rc/function/RC_Dec/Cycle_BDec_00040.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Cycle_BDec_00040 in RC测试-Cycle-00.vsd
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: Cycle_BDec_00040.java
 *- @ExecuteClass: Cycle_BDec_00040
 *- @ExecuteArgs:
 *- @Remark:
 */
class Cycle_BDec_00040_A1 {
    Cycle_BDec_00040_A2 a2_0;
    int a;
    int sum;

    Cycle_BDec_00040_A1() {
        a2_0 = null;
        a = 1;
        sum = 0;
    }

    void add() {
        sum = a + a2_0.a;
    }
}


class Cycle_BDec_00040_A2 {
    Cycle_BDec_00040_A3 a3_0;
    Cycle_BDec_00040_A4 a4_0;
    int a;
    int sum;

    Cycle_BDec_00040_A2() {
        a3_0 = null;
        a4_0 = null;
        a = 2;
        sum = 0;
    }

    void add() {
        sum = a + a3_0.a;
    }
}


class Cycle_BDec_00040_A3 {
    Cycle_BDec_00040_A1 a1_0;
    Cycle_BDec_00040_A4 a4_0;
    int a;
    int sum;

    Cycle_BDec_00040_A3() {
        a1_0 = null;
        a4_0 = null;
        a = 3;
        sum = 0;
    }

    void add() {
        sum = a + a1_0.a;
    }
}


class Cycle_BDec_00040_A4 {
    Cycle_BDec_00040_A3 a3_0;
    int a;
    int sum;

    Cycle_BDec_00040_A4() {
        a3_0 = null;
        a = 4;
        sum = 0;
    }

    void add() {
        sum = a + a3_0.a;
    }
}

public class Cycle_BDec_00040 {

    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
        System.out.println("ExpectResult");
    }

    private static void modifyA1(Cycle_BDec_00040_A1 a1) {
        a1.add();
        a1.a2_0.add();
        a1.a2_0.a3_0.add();
        a1.a2_0.a3_0.a4_0.add();
    }

    private static void rc_testcase_main_wrapper() {
        Cycle_BDec_00040_A1 a1_0 = new Cycle_BDec_00040_A1();
        a1_0.a2_0 = new Cycle_BDec_00040_A2();
        a1_0.a2_0.a3_0 = new Cycle_BDec_00040_A3();
        Cycle_BDec_00040_A4 a4_0 = new Cycle_BDec_00040_A4();
        a1_0.a2_0.a3_0.a1_0 = a1_0;
        a4_0.a3_0 = a1_0.a2_0.a3_0;
        a1_0.a2_0.a3_0.a4_0 = a4_0;
        modifyA1(a1_0);
        a1_0 = null;
        a4_0 = null;
    }

}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
