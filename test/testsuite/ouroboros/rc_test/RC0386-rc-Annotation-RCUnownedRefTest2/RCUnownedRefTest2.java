/*
 *- @TestCaseID:rc/unownedRef/RCUnownedRefTest2.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Test the basic function of @Unowned
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RCUnownedRefTest2.java
 *- @ExecuteClass: RCUnownedRefTest2
 *- @ExecuteArgs:
 */

import com.huawei.ark.annotation.Unowned;

class Test_B {
    @Unowned
    Test_B bself;

    protected void run() {
        System.out.println("ExpectResult");
    }
}

class Test_A {

    Test_B bb;

    Test_B bb2;

    public void test() {
        foo();
        bb.bself.run();
        bb2.bself.run();
    }

    private void foo() {
        bb = new Test_B();
        bb2 = new Test_B();
        bb.bself = bb;
        bb2.bself = bb;
    }
}

public class RCUnownedRefTest2 {
    public static void main(String[] args) {
        new Test_A().test();
    }
}



// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
