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
public class DeadReferenceTest {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    static Reference weakRp;
    static ReferenceQueue rq = new ReferenceQueue();
    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            deadReferenceTest();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }
    static void setWeakRef() {
        weakRp = new WeakReference<Object>(null, rq);
    }
    static void deadReferenceTest() throws InterruptedException {
        Reference reference;
        setWeakRef();
        new Thread(new ThreadRf()).start();
        Thread.sleep(2000);
        if (weakRp.get() != null) {
            judgeNum++;
        }
        while ((reference = rq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                judgeNum++;
            }
        }
    }
}
class ThreadRf implements Runnable {
    public void run() {
        for (int i = 0; i < 60; i++) {
            WeakReference wr = new WeakReference(new Object());
            try {
                Thread.sleep(100);
            } catch (Exception e) {
                System.out.println(" sleep was Interrupted");
            }
        }
    }
}
