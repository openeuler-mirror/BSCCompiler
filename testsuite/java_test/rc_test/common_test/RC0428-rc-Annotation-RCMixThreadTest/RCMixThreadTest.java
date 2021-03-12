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
import java.util.ArrayList;
import com.huawei.ark.annotation.*;
public class RCMixThreadTest {
    public static void main(String[] args) throws InterruptedException {
        rc_testcase_main_wrapper();
    }
    public static void rc_testcase_main_wrapper() throws InterruptedException {
        RCMixTest_Weak rcMixTest_weak = new RCMixTest_Weak();
        RCMixTest_Weak rcMixTest_weak2 = new RCMixTest_Weak();
        RCMixTest_Weak rcMixTest_weak3 = new RCMixTest_Weak();
        RCMixTest_Weak rcMixTest_weak4 = new RCMixTest_Weak();
        RCMixTest_Weak rcMixTest_weak5 = new RCMixTest_Weak();
        rcMixTest_weak.start();
        rcMixTest_weak2.start();
        rcMixTest_weak3.start();
        rcMixTest_weak4.start();
        rcMixTest_weak5.start();
        rcMixTest_weak.join();
        rcMixTest_weak2.join();
        rcMixTest_weak3.join();
        rcMixTest_weak4.join();
        rcMixTest_weak5.join();
    }
}
class RCMixTest_Weak extends Thread {
    public void run() {
        new Test_A_Weak().test();
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
