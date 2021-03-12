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
public class RCPermanentThread3 {
    static {
        System.loadLibrary("jniTestHelper");
    }
    public static void main(String[] args) throws InterruptedException {
        rc_testcase_main_wrapper();
        rc_testcase_main_wrapper2();
    }
    private static void rc_testcase_main_wrapper() throws InterruptedException {
        RCPermanentTest rcPermanentTest = new RCPermanentTest();
        RCPermanentTest rcPermanentTest2 = new RCPermanentTest();
        RCPermanentTest rcPermanentTest3 = new RCPermanentTest();
        RCPermanentTest rcPermanentTest4 = new RCPermanentTest();
        RCPermanentTest rcPermanentTest5 = new RCPermanentTest();
        rcPermanentTest.start();
        rcPermanentTest.join();
        rcPermanentTest2.start();
        rcPermanentTest2.join();
        rcPermanentTest3.start();
        rcPermanentTest3.join();
        rcPermanentTest4.start();
        rcPermanentTest4.join();
        rcPermanentTest5.start();
        rcPermanentTest5.join();
        // rcPermanentTest.join();
        // rcPermanentTest2.join();
        // rcPermanentTest3.join();
        // rcPermanentTest4.join();
        // rcPermanentTest5.join();
        if (rcPermanentTest.check() && rcPermanentTest2.check() && rcPermanentTest3.check() && rcPermanentTest4.check() && rcPermanentTest5.check()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("rcPermanentTest.check() :" + rcPermanentTest.check());
            System.out.println("rcPermanentTest2.check() :" + rcPermanentTest2.check());
            System.out.println("rcPermanentTest3.check() :" + rcPermanentTest3.check());
            System.out.println("rcPermanentTest4.check() :" + rcPermanentTest4.check());
            System.out.println("rcPermanentTest5.check() :" + rcPermanentTest5.check());
        }
    }
    private static void rc_testcase_main_wrapper2() throws InterruptedException {
        RCWeakTest rcWeakTest = new RCWeakTest();
        RCWeakTest rcWeakTest2 = new RCWeakTest();
        RCWeakTest rcWeakTest3 = new RCWeakTest();
        RCWeakTest rcWeakTest4 = new RCWeakTest();
        RCWeakTest rcWeakTest5 = new RCWeakTest();
        rcWeakTest.start();
        rcWeakTest2.start();
        rcWeakTest3.start();
        rcWeakTest4.start();
        rcWeakTest5.start();
        rcWeakTest.join();
        rcWeakTest2.join();
        rcWeakTest3.join();
        rcWeakTest4.join();
        rcWeakTest5.join();
    }
}
class RCPermanentTest extends Thread {
    public boolean checkout;
    public static native boolean isHeapObject(Object obj);
    public static native int refCount(Object obj);
    public static int checkNum = 0;
    static Object owner;
    static boolean checkRC(Object obj) {
        int rc1 = refCount(obj);
        owner = obj;
        int rc2 = refCount(obj);
        owner = null;
        int rc3 = refCount(obj);
        if (rc1 != rc3) {
            throw new RuntimeException("rc incorrect!");
        }
        if (rc1 == rc2 && rc2 == rc3) {
            //如果相等，说明annotation生效，没有经过RC处理
            return false;
        } else {
            return true;
        }
        //System.out.printf("rc:%-5s heap:%-5s %s%n", !skipRC, isHeapObject(obj), title);
    }
    /*
    验证new int @Permanent [8]
   */


    static void method1(Object obj) {
        obj = new int@Permanent[8];
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new int @Permanent [8];in method1");
        }
    }
    /*
    验证new int [8]
   */


    static void method2(Object obj) {
        obj = new int[8];
        boolean result1 = checkRC(obj);
        if (result1 == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in new int [8];in method2");
        }
    }
    /*
    验证new @Permanent int[8]
  */


    static void method3(Object obj) {
        obj = new @Permanent int[1];
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new @Permanent int[8];in method3");
        }
    }
    /*
    验证new Integer @Permanent [10]
  */


    static void method4(Object obj) {
        Integer[] arr = new Integer@Permanent[10];
        boolean result1 = checkRC(obj);
        noleak.add(arr); // let leak check happy
        arr[0] = new Integer(10000);
        arr[9] = new Integer(20000);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new Integer @Permanent [10];in method4");
        }
    }
    /*
   验证new @Permanent Integer  [10]
 */


    static void method5(Object obj) {
        Integer[] arr = new @Permanent Integer[10];
        boolean result1 = checkRC(obj);
        noleak.add(arr); // let leak check happy
        arr[0] = new Integer(10000);
        arr[9] = new Integer(20000);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new @Permanent Integer [10];in method5");
        }
    }
    /*
    验证new Integer[10];
  */


    static void method6(Object obj) {
        obj = new Integer[10];
        boolean result1 = checkRC(obj);
        if (result1 == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in new Integer[10];in method6");
            System.out.println("checkRC(obj):" + result1 + "        isHeapObject(obj):" + isHeapObject(obj));
        }
    }
    /*
    验证new @Permanent ArrayList[]{};
  */


    static void method7(Object obj) {
        obj = new @Permanent ArrayList[]{};
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new @Permanent ArrayList[]{};in method7");
        }
    }
    /*
    验证new ArrayList @Permanent []{};
  */


    static void method8(Object obj) {
        obj = new ArrayList@Permanent[]{};
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new ArrayList @Permanent []{};in method8");
        }
    }
    /*
    验证new ArrayList []{};
  */


    static void method9(Object obj) {
        obj = new ArrayList[]{};
        boolean result1 = checkRC(obj);
        if (result1 == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in new ArrayList []{}; method9");
        }
    }
    static ArrayList<Object> noleak = new ArrayList<>();
    public void run(
    ) {
        checkNum = 0;
        Object obj = null;
        method1(obj);
        method2(obj);
        method3(obj);
        method4(obj);
        method5(obj);
        method6(obj);
        method7(obj);
        method8(obj);
        method9(obj);
        // System.out.println(checkNum);
        if (checkNum == 9) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println("checkout== false,checkNum:"+checkNum);
        }
    }
    public boolean check() {
        return checkout;
    }
}
class RCWeakTest extends Thread {
    public void run() {
        new Test_A().test();
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
class Test_B {
    public void run() {
        System.out.println("ExpectResult");
    }
}
