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
import java.util.LinkedList;
import java.util.List;
class RCUnownedLocalTest extends Thread {
    private boolean checkout;
    Integer a = new Integer(1);
    Object[] arr = new Object[]{1, 2, 3};
    @UnownedLocal
    int method(Integer a, Object[] arr) {
        int check = 0;
        Integer c = a + a;
        if (c == 2) {
            check++;
        } else {
            check--;
        }
        for (Object array : arr) {
            //System.out.println(array);
            check++;
        }
        return check;
    }
    public void run() {
        int result = method(a, arr);
        if (result == 4) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println("result:" + result);
        }
    }
    public boolean check() {
        return checkout;
    }
}
public class RCUnownedLocalThreadTest {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        RCUnownedLocalTest rcTest1 = new RCUnownedLocalTest();
        RCUnownedLocalTest rcTest2 = new RCUnownedLocalTest();
        RCUnownedLocalTest rcTest3 = new RCUnownedLocalTest();
        RCUnownedLocalTest rcTest4 = new RCUnownedLocalTest();
        RCUnownedLocalTest rcTest5 = new RCUnownedLocalTest();
        RCUnownedLocalTest rcTest6 = new RCUnownedLocalTest();
        rcTest1.start();
        rcTest2.start();
        rcTest3.start();
        rcTest4.start();
        rcTest5.start();
        rcTest6.start();
        try {
            rcTest1.join();
            rcTest2.join();
            rcTest3.join();
            rcTest4.join();
            rcTest5.join();
            rcTest6.join();
        } catch (InterruptedException e) {
        }
        if (rcTest1.check() && rcTest2.check() && rcTest3.check() && rcTest4.check() && rcTest5.check() && rcTest6.check())
            System.out.println("ExpectResult");
    }
}
