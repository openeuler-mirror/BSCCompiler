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


import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
public class NotIsCleanerFreeRef {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer obj = new StringBuffer("soft");
    static void setSoftRef() {
        obj = new StringBuffer("soft");
        rp = new WeakReference<Object>(obj, rq);
        if (rp.get() == null) {
            judgeNum++;
        }
        obj = null;
    }
    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            notIsCleanerFreeRef();
            Runtime.getRuntime().gc();
            notIsCleanerFreeRef();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }
    static void notIsCleanerFreeRef() throws InterruptedException {
        Reference ref;
        setSoftRef();
        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        if (rp.get() != null) {
            judgeNum++;
        }
        while ((ref = rq.poll()) != null) {
            if (!ref.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                judgeNum++;
            }
        }
    }
}
class ThreadRef implements Runnable {
    public void run() {
        for (int i = 0; i < 60; i++) {
            WeakReference wr = new WeakReference(new Object());
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                System.out.println(" sleep was Interrupted");
            }
        }
    }
}