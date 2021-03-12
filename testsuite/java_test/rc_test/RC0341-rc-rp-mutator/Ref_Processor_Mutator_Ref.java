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
public class Ref_Processor_Mutator_Ref {
    static int check_count = 0;
    static final int TEST_NUM = 1;
    static Reference wr, sr, pr;
    static ReferenceQueue wrq, srq, prq;
    static StringBuffer stringBuffer1 = new StringBuffer("weak");
    static StringBuffer stringBuffer2 = new StringBuffer("soft");
    static StringBuffer stringBuffer3 = new StringBuffer("phantom");
    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        Runtime.getRuntime().gc();
        test();
        if (check_count == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error,check_count: " + check_count);
        }
    }
    private static void test() {
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
            sleep(2000);
            if (wr.get() != null) {
                check_count++;
            }
            if (sr.get() != null) {
                check_count++;
            }
        }
        Reference wrqPoll, srqPoll, prqPoll;
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                check_count++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in " +
                        "wrq.poll()");
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                check_count++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in " +
                        "srq.poll()");
            }
        }
        while ((prqPoll = prq.poll()) != null) {
            if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                check_count++;
                System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in " +
                        "prq.poll()");
            }
        }
    }
    public static void test_01() {
        wrq = new ReferenceQueue();
        srq = new ReferenceQueue();
        prq = new ReferenceQueue();
        stringBuffer1 = new StringBuffer("weak");
        stringBuffer2 = new StringBuffer("soft");
        stringBuffer3 = new StringBuffer("phantom");
        wr = new WeakReference<StringBuffer>(stringBuffer1, wrq);
        if (wr.get() == null) {
            check_count++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ----------------------" +
                    "check_count: " + check_count);
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in wr");
        }
        sr = new WeakReference<StringBuffer>(stringBuffer2, srq);
        if (sr.get() == null) {
            check_count++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ----------------------" +
                    "check_count: " + check_count);
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in sr");
        }
        pr = new PhantomReference<StringBuffer>(stringBuffer3, prq);
        if (pr.get() != null) {
            check_count++;
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ----------------------" +
                    "check_count: " + check_count);
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " ErrorResult in pr");
        }
        stringBuffer1 = null;
        stringBuffer2 = null;
        stringBuffer3 = null;
    }
    private static void sleep(int sleepNum) {
        try {
            Thread.sleep(sleepNum);
        } catch (InterruptedException e) {
            System.out.println(Thread.currentThread().getStackTrace()[2].getClassName() + " sleep was Interrupted");
        }
    }
}
