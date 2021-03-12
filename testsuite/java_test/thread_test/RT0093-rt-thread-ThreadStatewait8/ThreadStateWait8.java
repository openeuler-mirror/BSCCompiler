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


public class ThreadStateWait8 extends Thread {
    static int k = 0;
    public static void main(String[] args) {
        Object object = "aa";
        Thread[] tt = new Thread[10];
        for (int i = 0; i < tt.length; i++) {
            tt[i] = new Thread() {
                public void run() {
                    synchronized (object) {
                        try {
                            object.wait();
                        } catch (InterruptedException e1) {
                            k++;
                        }
                    }
                }
            };
        }
        for (int i = 0; i < tt.length; i++) {
            tt[i].start();
        }
        for (int i = 0; i < tt.length; i++) {
            tt[i].interrupt();
        }
        for (int i = 0; i < tt.length; i++) {
            try {
                tt[i].join();
            } catch (InterruptedException e2) {
                System.err.println(e2);
            }
        }
        if (k == 10) {
            System.out.println(0);
        }
    }
}