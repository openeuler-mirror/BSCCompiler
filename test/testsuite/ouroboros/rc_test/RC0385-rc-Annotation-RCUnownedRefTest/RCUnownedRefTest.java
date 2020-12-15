/*
 *- @TestCaseID:rc/unownedRef/RCUnownedRefTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Test the basic function with cycle of the @Unowned
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCUnownedRefTest.java
 *- @ExecuteClass: RCUnownedRefTest
 *- @ExecuteArgs:
 *- @Remark:Owner Zhang Wennlong 00292413
 */

import java.lang.reflect.Field;
import com.huawei.ark.annotation.Unowned;

class Test_B {
    @Unowned
    Test_A aa;

    // add volatile will crash
    // static Test_A a1;

    protected void finalize() {
        System.out.println("ExpectResult");
    }
}

class Test_A {
    Test_B bb;

    public void test() {
        setReferences();
        System.runFinalization();
    }

    private void setReferences() {
        Test_A ta = new Test_A();
        ta.bb = new Test_B();
        //ta.bb.aa = ta;

        try {
            Field m = Test_B.class.getDeclaredField("aa");
            m.set(ta.bb, ta);
            Test_A a_temp = (Test_A) m.get(ta.bb);
            if (a_temp != ta) {
                System.out.println("error");
            }
            //Field m1 = Test_B.class.getDeclaredField("a1");
            //m1.set(null, ta);
        } catch (Exception e) {
            System.out.println(e);
        }
    }
}


public class RCUnownedRefTest {
    public static void main(String[] args) {
        new Test_A().test();
    }
}



// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
