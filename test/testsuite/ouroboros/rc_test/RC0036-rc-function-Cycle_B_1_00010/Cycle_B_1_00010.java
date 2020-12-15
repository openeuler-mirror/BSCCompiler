/*
 *- @TestCaseID:maple/runtime/rc/function/Cycle_B_1_00010.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Cycle_B_1_00010 in RC测试-Cycle-00.vsd
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: Cycle_B_1_00010.java
 *- @ExecuteClass: Cycle_B_1_00010
 *- @ExecuteArgs:
 *- @Remark:
 */
class Cycle_B_1_00010_A1 {
    Cycle_B_1_00010_A1 a1_0;
    int a;
    int sum;

    Cycle_B_1_00010_A1() {
        a1_0 = null;
        a = 123;
        sum = 0;
    }

    void add() {
        sum = a + a1_0.a;
    }
}

public class Cycle_B_1_00010 {

    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
        
    }

    private static void rc_testcase_main_wrapper() {
        Cycle_B_1_00010_A1 a1_0 = new Cycle_B_1_00010_A1();
        a1_0.a1_0 = a1_0;
        a1_0.add();
        int nsum = a1_0.sum;
        if (nsum == 246)
            System.out.println("ExpectResult");
    }

}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
