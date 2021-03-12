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
