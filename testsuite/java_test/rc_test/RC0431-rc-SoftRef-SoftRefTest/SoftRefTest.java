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
import java.lang.ref.SoftReference;
public class SoftRefTest {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int a = 100;
    static void setSoftReference() {
        rp = new SoftReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            System.out.println("Error Result when first check ");
            a++;
        }
    }
    public static void main(String[] args) throws Exception {
        setSoftReference();
        new Thread(new TriggerRP()).start();
        if (rp.get() == null) {
            System.out.println("Error Result when second check ");
            a++;
        }
        Reference r = rq.poll();
        if (r != null) {
            System.out.println("Error Result when checking ReferenceQueue");
            a++;
        }
        if (a == 100) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult finally");
        }
    }
    static class TriggerRP implements Runnable {
        public void run() {
            for (int i = 0; i < 60; i++) {
                SoftReference sr = new SoftReference(new Object());
                try {
                    Thread.sleep(50);
                } catch (Exception e) {
                }
            }
        }
    }
}
