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


import java.lang.ref.*;
public class Ref_Processor_Weak_Ref_Boundary {
    static int checkCount = 0;
    static final int TEST_NUM = 1;
    static WeakReference wr[] = new WeakReference[14];
    static WeakReference sr[] = new WeakReference[14];
    static PhantomReference pr[] = new PhantomReference[14];
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static StringBuffer stringBuffer1 = new StringBuffer("soft");
    static StringBuffer stringBuffer2 = new StringBuffer("weak");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");
    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        //Result judgment
        if (checkCount == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("errorResult,checkNum: " + checkCount);
        }
    }
    private static void test() {
        //test01: 14 mix weak references
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
        }
        sleep(1000);
    }
    public static void setSoftRef(int times) {
        stringBuffer1 = new StringBuffer("soft");
        for (int i = 0; i < times; i++) {
            sr[i] = new WeakReference<Object>(stringBuffer1, srq);
            if (sr[i].get() == null) {
                System.out.println(" ---------------error in sr of test01");
                checkCount++;
            }
        }
        stringBuffer1 = null;
    }
    public static void setWeakRef(int times) {
        stringBuffer2 = new StringBuffer("Weak");
        for (int i = 0; i < times; i++) {
            wr[i] = new WeakReference<Object>(stringBuffer2, wrq);
            if (wr[i].get() == null) {
                System.out.println(" ---------------error in wr of test01");
                checkCount++;
            }
        }
        stringBuffer2 = null;
    }
    public static void setPhantomRef(int times) {
        stringBuffer3 = new StringBuffer("phantom");
        for (int i = 0; i < times; i++) {
            pr[i] = new PhantomReference<Object>(stringBuffer3, prq);
            if (pr[i].get() != null) {
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ---------------" +
                        "error in pr of test01");
                checkCount++;
            }
        }
        stringBuffer3 = null;
    }
    public static void test_01() {
        /* 14 mix weak references*/

        setSoftRef(5);
        checkSoftRq("soft", 5);
        setWeakRef(4);
        checkWeakRq("weak", 4);
        setPhantomRef(5);
        checkPhantomRq("phantom");
        setSoftRef(5);
        checkSoftRq("soft", 5);
        setWeakRef(5);
        checkWeakRq("weak", 5);
        setPhantomRef(5);
        checkPhantomRq("phantom");
        setSoftRef(5);
        checkSoftRq("soft", 5);
        setWeakRef(6);
        checkWeakRq("weak", 6);
        setPhantomRef(5);
        checkPhantomRq("phantom");
    }
    public static void checkSoftRq(String funName, int times) {
        Reference srqPoll;
        sleep(2000);
        if (sr != null) {
            for (int i = 0; i < times; i++) {
                if (sr[i].get() != null) {
                    checkCount++;
                    System.out.println(" ErrorResult in sr      " + funName);
                }
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in srq.poll()");
            }
        }
    }
    public static void checkWeakRq(String funName, int times) {
        Reference wrqPoll;
        sleep(2000);
        for (int i = 0; i < times; i++) {
            if (wr[i].get() != null) {
                checkCount++;
                System.out.println(" ErrorResult in wr       " + funName + "times:" + i);
            }
        }
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in wrq.poll()");
            }
        }
    }
    public static void checkPhantomRq(String funName) {
        Reference prqPoll;
        while ((prqPoll = prq.poll()) != null) {
            if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                checkCount++;
                System.out.println(" ErrorResult in prq.poll()");
            }
        }
    }
    private static void sleep(int sleepNum) {
        try {
            Thread.sleep(sleepNum);
        } catch (InterruptedException e) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " sleep was Interrupted");
        }
    }
}
