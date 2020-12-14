/*
 *- @TestCaseID:maple/runtime/rc/function/RCWeakRefTest4.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Cycle_B_1_00010 for @Weak Test
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCWeakRefTest4.java
 *- @ExecuteClass: RCWeakRefTest4
 *- @ExecuteArgs:
 *- @Remark: Owner Zhang Wenlong 00292413
 */

import com.huawei.ark.annotation.Weak;

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
        sum = a + a1_0.a;
    }
}

public class RCWeakRefTest4 {

    public static void main(String[] args) {
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
