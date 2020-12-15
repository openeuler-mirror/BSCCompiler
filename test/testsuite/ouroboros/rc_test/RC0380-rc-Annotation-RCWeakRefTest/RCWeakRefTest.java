/*
 *- @TestCaseID:rc/unownedRef/RCWeakRefTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Test the basic function of @Weak
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RCWeakRefTest.java
 *- @ExecuteClass: RCWeakRefTest
 *- @ExecuteArgs:
 */

import com.huawei.ark.annotation.Weak;

class Test_B {
    public void run() {
        System.out.println("ExpectResult");
    }
}

class Test_A {
    @Weak
    Test_B bb;
    Test_B bb2;

    public void test() {
        foo();
        try {
            Thread.sleep(5000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            bb.run();
        } catch (NullPointerException e) {
            System.out.println("ExpectResult");
        }
        bb2.run();
    }

    private void foo() {
        bb = new Test_B();
        bb2 = new Test_B();
    }
}

public class RCWeakRefTest {
    public static void main(String[] args) {
        new Test_A().test();
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
