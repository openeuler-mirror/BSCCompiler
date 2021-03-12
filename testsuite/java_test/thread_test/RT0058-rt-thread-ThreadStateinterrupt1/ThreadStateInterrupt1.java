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


public class ThreadStateInterrupt1 extends Thread {
    static int i = 0;
    static boolean j;
    public static void main(String[] args) {
        ThreadStateInterrupt1 cls = new ThreadStateInterrupt1();
        cls.start();
        try {
            sleep(50);
        } catch (InterruptedException e2) {
            System.err.println(e2);
        }
        cls.interrupt();
        try {
            sleep(50);
        } catch (InterruptedException e3) {
            System.err.println(e3);
        }
        if (j && i == 0) {
            System.out.println(0);
        }
    }
    public synchronized void run() {
        try {
            while (!Thread.currentThread().isInterrupted()) {
                wait();
                i++;
            }
        } catch (InterruptedException e1) {
            Thread.currentThread().interrupt();
            j = Thread.currentThread().isInterrupted();
        }
    }
}