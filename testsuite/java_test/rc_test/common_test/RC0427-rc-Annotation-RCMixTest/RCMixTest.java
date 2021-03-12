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