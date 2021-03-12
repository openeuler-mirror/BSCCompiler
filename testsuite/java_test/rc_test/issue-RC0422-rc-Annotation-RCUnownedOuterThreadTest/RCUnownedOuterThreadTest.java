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
class RCUnownedOuterTest extends Thread {
    private boolean checkout;
    public void run() {
        UnownedAnnoymous unownedAnnoymous = new UnownedAnnoymous();
        unownedAnnoymous.anonymousCapture();
        UnownedInner unownedInner = new UnownedInner();
        unownedInner.method();
        UnownedInner.InnerClass innerClass = unownedInner.new InnerClass();
        innerClass.myName();
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (unownedAnnoymous.checkAnnoy == true && unownedInner.checkInner == true) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println("error");
            System.out.println("unownedAnnoymous.checkAnnoy:" + unownedAnnoymous.checkAnnoy);
            System.out.println("unownedInner.checkInner:" + unownedInner.checkInner);
        }
    }
    public boolean check() {
        return checkout;
    }
}
class UnownedInner {
    String name = "test";
    boolean checkInner = false;
    InnerClass innerClass;
    void method() {
        innerClass = new InnerClass();
    }
    @UnownedOuter
    class InnerClass {
        void myName() {
            checkInner = false;
            String myname;
            myname = name + name;
            if (myname.equals("testtest")) {
                checkInner=true;
               // System.out.println("checkInner:"+checkInner);
            }
        }
    }
}
class UnownedAnnoymous {
    String name = "test";
    boolean checkAnnoy=false;
    void anonymousCapture() {
        Runnable r = new Runnable() {
            @UnownedOuter
            @Override
            public void run() {
                checkAnnoy = false;
                String myName = name + name;
                if (myName.equals("testtest")) {
                    checkAnnoy=true;
                  // System.out.println("checkAnnoy:"+checkAnnoy);
                }
            }
        };
        r.run();
    }
}
public class RCUnownedOuterThreadTest {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        RCUnownedOuterTest rcTest1 = new RCUnownedOuterTest();
        RCUnownedOuterTest rcTest2 = new RCUnownedOuterTest();
        RCUnownedOuterTest rcTest3 = new RCUnownedOuterTest();
        RCUnownedOuterTest rcTest4 = new RCUnownedOuterTest();
        RCUnownedOuterTest rcTest5 = new RCUnownedOuterTest();
        RCUnownedOuterTest rcTest6 = new RCUnownedOuterTest();
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
        if (rcTest1.check() && rcTest2.check() && rcTest3.check() && rcTest4.check() && rcTest5.check() && rcTest6.check()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("rcTest1.check():" + rcTest1.check());
            System.out.println("rcTest2.check():" + rcTest2.check());
            System.out.println("rcTest3.check():" + rcTest3.check());
            System.out.println("rcTest4.check():" + rcTest4.check());
            System.out.println("rcTest5.check():" + rcTest5.check());
            System.out.println("rcTest6.check():" + rcTest6.check());
        }
    }
}
