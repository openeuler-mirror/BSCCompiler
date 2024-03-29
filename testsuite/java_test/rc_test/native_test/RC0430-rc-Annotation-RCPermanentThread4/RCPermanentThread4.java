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


import com.huawei.ark.annotation.Permanent;
public class RCPermanentThread4 {
    static {
        System.loadLibrary("jniTestHelper");
    }
    public static void main(String[] args) throws InterruptedException {
        rc_testcase_main_wrapper();
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
        SyncTest syncTest = new SyncTest();
        SyncTest syncTest1 = new SyncTest();
        SyncTest syncTest2 = new SyncTest();
        SyncTest syncTest3 = new SyncTest();
        SyncTest syncTest4 = new SyncTest();
        syncTest.start();
        syncTest.join();
        syncTest1.start();
        syncTest1.join();
        syncTest2.start();
        syncTest2.join();
        syncTest3.start();
        syncTest3.join();
        syncTest4.start();
        syncTest4.join();
        if (syncTest.check() && syncTest1.check() && syncTest2.check() && syncTest3.check() && syncTest4.check()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("rcPermanentTest.check() :" + syncTest.check());
            System.out.println("rcPermanentTest2.check() :" + syncTest1.check());
            System.out.println("rcPermanentTest3.check() :" + syncTest2.check());
            System.out.println("rcPermanentTest4.check() :" + syncTest3.check());
            System.out.println("rcPermanentTest5.check() :" + syncTest4.check());
        }
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
        try {
            obj = new int@Permanent[8];
            boolean result1 = checkRC(obj);
            if (result1 == false && isHeapObject(obj) == false) {
                checkNum++;
            } else {
                System.out.println("error in new int @Permanent [8];in method1");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    public void run() {
        checkNum = 0;
        Object obj = null;
        method1(obj);
        if (checkNum == 1) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println(checkNum);
        }
    }
    public boolean check() {
        return checkout;
    }
}
class SyncTest extends Thread {
    public boolean checkout;
    public static native boolean isHeapObject(Object obj);
    public static native int refCount(Object obj);
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
    public void run() {
        synchronized (this) {
            Object obj = null;
            obj = new int@Permanent[8];
            boolean result1 = checkRC(obj);
            if (result1 == false && isHeapObject(obj) == false) {
                checkout = true;
            } else {
                checkout = false;
                System.out.println("error in new int @Permanent [8];in method1");
            }
        }
    }
    public boolean check() {
        return checkout;
    }
}