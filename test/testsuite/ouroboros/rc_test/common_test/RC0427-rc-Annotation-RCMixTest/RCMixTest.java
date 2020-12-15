/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentTest6
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Weak 或Unowned和Permanent一起用在一个对象的case，验证是否能够正常被释放
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * Weak 或Unowned和Permanent一起用在一个对象的case，验证是否能够正常被释放
 *- @Expect:ExpectResult\nExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RCMixTest.java
 *- @ExecuteClass: RCMixTest
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */

import com.huawei.ark.annotation.*;

import java.lang.reflect.Field;


public class RCMixTest {


    public static void main(String[] args) {

        new Test_A_Weak().test();
        new Test_A_Unowned().test();
        Runtime.getRuntime().gc();
    }
}

class Test_A_Weak {
    @Weak
    Test_B_Weak bb;
    Test_B_Weak bb2;

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
            System.out.println("NullPointerException");
        }
        bb2.run();
    }

    private void foo() {
        bb = new @Permanent Test_B_Weak();
        bb2 = new Test_B_Weak();
    }
}

class Test_B_Weak {
    public void run() {
        System.out.println("ExpectResult");
    }
}


class Test_B_Unowned {
    @Unowned
    Test_A_Unowned aa ;

    // add volatile will crash
    // static Test_A a1;

    protected void finalize() {
        System.out.println("ExpectResult");
    }
}

class Test_A_Unowned {
    Test_B_Unowned bb;

    public void test() {
        setReferences();
        System.runFinalization();
    }

    private void setReferences() {
        @Unowned
        Test_A_Unowned ta;
        ta = new @Permanent Test_A_Unowned();
        ta.bb = new Test_B_Unowned();
        //ta.bb.aa = ta;

        try {
            Field m = Test_B_Unowned.class.getDeclaredField("aa");
            m.set(ta.bb, ta);
            Test_A_Unowned a_temp = (Test_A_Unowned) m.get(ta.bb);
            if (a_temp != ta) {
                System.out.println("error");
            }
        } catch (Exception e) {
            System.out.println(e);
        }

    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
