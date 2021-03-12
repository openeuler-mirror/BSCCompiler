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
public class Ref_Processor_Single_Or_More_Weak_Ref {
    static int check_count = 0;
    static final int TEST_NUM = 1;
    static Reference wr, sr, pr;
    static ReferenceQueue wrq, srq, prq;
    static StringBuffer obj_1 = new StringBuffer("hello");
    static StringBuffer obj_2 = new StringBuffer("kitty");
    static StringBuffer obj_3 = new StringBuffer("mikey");
    static StringBuffer obj4 = new StringBuffer("WeakReference");
    static StringBuffer obj5 = new StringBuffer("WeakReference");
    static StringBuffer obj6 = new StringBuffer("PhantomReference");
    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        if (check_count == 0)
            System.out.println("ExpectResult");
        else {
            System.out.println("ErrorResult, check_count:" + check_count);
        }
    }
    private static void test() {
        Reference wrqPoll, srqPoll, prqPoll;
        for (int i = 0; i < TEST_NUM; i++) {
            weakRef();
            sleep(2000);
            if (wr.get() != null) {
                check_count++;
                System.out.println(" ErrorResult in weakRef step1");
            }
            while ((wrqPoll = wrq.poll()) != null) {
                if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in wrq.poll() step2");
                }
            }
        }
        // check WeakReference
        for (int i = 0; i < TEST_NUM; i++) {
            softRef();
            sleep(2000);
            if (sr.get() != null) {
                check_count++;
                System.out.println(" ErrorResult in softRef step2");
            }
            while ((srqPoll = srq.poll()) != null) {
                if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in srq.poll()");
                }
            }
        }
        // check PhantomReference
        for (int i = 0; i < TEST_NUM; i++) {
            phantomRef();
            sleep(2000);
            while ((prqPoll = prq.poll()) != null) {
                if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in prq.poll()");
                }
            }
        }
        // more than one reference
        for (int i = 0; i < TEST_NUM; i++) {
            mixRef();
            sleep(2000);
            if (wr.get() != null) {
                check_count++;
                System.out.println(" ErrorResult in mixRef step2:weakRef");
            }
            if (sr.get() != null) {
                check_count++;
                System.out.println(" ErrorResult in mixRef step2:softRef");
            }
            if (pr.get() != null) {
                check_count++;
                System.out.println(" ErrorResult in mixRef step2:phantomRef");
            }
            while ((wrqPoll = wrq.poll()) != null) {
                if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in wrq.poll()");
                }
            }
            while ((srqPoll = srq.poll()) != null) {
                if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in srq.poll()");
                }
            }
            while ((prqPoll = prq.poll()) != null) {
                if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in prq.poll()");
                }
            }
        }
    }
    private static void weakRef() {
        wrq = new ReferenceQueue();
        obj_1 = new StringBuffer("hello");
        wr = new WeakReference<StringBuffer>(obj_1, wrq);
        if (wr.get() == null) {
            check_count++;
            System.out.println(" ErrorResult in weakRef step1");
        }
        obj_1 = null;
    }
    private static void softRef() {
        srq = new ReferenceQueue();
        obj_2 = new StringBuffer("kitty");
        sr = new WeakReference<StringBuffer>(obj_2, srq);
        if (sr.get() == null) {
            check_count++;
            System.out.println(" ErrorResult in softRef step1");
        }
        obj_2 = null;
    }
    private static void phantomRef() {
        obj_3 = new StringBuffer("mikey");
        prq = new ReferenceQueue();
        pr = new PhantomReference<StringBuffer>(obj_3, prq);
        if (pr.get() != null) {
            check_count++;
            System.out.println(" ErrorResult in phantomRef step1");
        }
        obj_3 = null;
    }
    private static void mixRef() {
        wrq = new ReferenceQueue();
        srq = new ReferenceQueue();
        prq = new ReferenceQueue();
        obj4 = new StringBuffer("WeakReference");
        obj5 = new StringBuffer("WeakReference");
        obj6 = new StringBuffer("PhantomReference");
        wr = new WeakReference<StringBuffer>(obj4, wrq);
        if (wr.get() == null) {
            check_count++;
            System.out.println(" ErrorResult of wr in mixRef step1");
        }
        sr = new WeakReference<StringBuffer>(obj5, srq);
        if (sr.get() == null) {
            check_count++;
            System.out.println(" ErrorResult of sr in mixRef step2");
        }
        pr = new PhantomReference<StringBuffer>(obj6, prq);
        if (pr.get() != null) {
            check_count++;
            System.out.println(" ErrorResult of pr in mixRef step3");
        }
        obj4 = null;
        obj5 = null;
        obj6 = null;
    }
    private static void sleep(int sleepNum) {
        try {
            Thread.sleep(sleepNum);
        } catch (InterruptedException e) {
            System.out.println(" sleep was Interrupted");
        }
    }
}