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
import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
public class Ref_Processor_Wcb_Test_Ref {
    static int checkCount = 0;
    static final int TEST_NUM = 1;
    static Reference wr, sr, pr;
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
        if (checkCount == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("errorResult ---- checkcount: " + checkCount);
        }
    }
    private static void test() {
        // test: if WCB == 0, return object value
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
            sleep(1000);
        }
        // test: if WCB == 1, return null
        checkRet("test step 2");
        // test: check change WCB 1 to 0
    }
    private static void test_01() {
        /*if WCB == 0, return object value except PhantomReference*/
        stringBuffer1 = new StringBuffer("soft");
        stringBuffer2 = new StringBuffer("weak");
        stringBuffer3 = new StringBuffer("phantom");
        wr = new WeakReference<Object>(stringBuffer2, wrq);
        if (wr.get() == null) {
            checkCount++;
        }
        sr = new WeakReference<Object>(stringBuffer1, srq);
        if (sr.get() == null) {
            checkCount++;
        }
        pr = new PhantomReference<Object>(stringBuffer3, prq);
        if (pr.get() != null) {
            checkCount++;
        }
        stringBuffer2 = null;
        stringBuffer1 = null;
        stringBuffer3 = null;
    }
    private static void checkRet(String funName) {
        sleep(2000);
        Reference wrqPoll, srqPoll, prqPoll;
        if (sr.get() != null) {
            checkCount++;
        }
        if (wr.get() != null) {
            checkCount++;
        }
        if (pr.get() != null) {
            checkCount++;
        }
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in wrq.poll()");
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println(" ErrorResult in srq.poll()");
            }
        }
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
