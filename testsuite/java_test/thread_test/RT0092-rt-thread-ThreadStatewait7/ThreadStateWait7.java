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


public class ThreadStateWait7 extends Thread {
    static Object object = "aa";
    public static void main(String[] args) {
        long j;
        long k;
        ThreadStateWait7 threadStateWait7 = new ThreadStateWait7();
        threadStateWait7.start();
        j = System.currentTimeMillis();
        try {
            threadStateWait7.join();
        } catch (InterruptedException ex) {
        }
        k = System.currentTimeMillis();
        if (k - j > 950 && k - j < 1050) {
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (object) {
            try {
                object.wait(1000);
            } catch (InterruptedException e) {
            }
        }
    }
}