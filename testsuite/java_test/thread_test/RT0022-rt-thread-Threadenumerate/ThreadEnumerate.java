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


public class ThreadEnumerate extends Thread {
    static Thread[] p;
    static int w, q;
    static Object ob = "aa";
    public ThreadEnumerate(ThreadGroup group, String name) {
        super(group, name);
    }
    public static void main(String[] args) {
        ThreadGroup cls1 = new ThreadGroup("god");
        for (int i = 0; i < 10; i++) {
            (new ThreadEnumerate(cls1, "banana" + i)).start();
        }
        try {
            sleep(500);
        } catch (InterruptedException e1) {
        }
        if (w == 10) {
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (ob) {
            q = activeCount();
            p = new Thread[q + 1];
            w = enumerate(p);
            try {
                ob.wait(1000);
            } catch (InterruptedException e) {
            }
        }
    }
}