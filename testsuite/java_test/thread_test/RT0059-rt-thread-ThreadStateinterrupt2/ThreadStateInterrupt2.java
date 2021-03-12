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


public class ThreadStateInterrupt2 extends Thread {
    static int i = 0;
    static boolean j = false;
    public static void main(String[] args) {
        ThreadStateInterrupt2 cls = new ThreadStateInterrupt2();
        cls.start();
        try {
            sleep(50);
        } catch (InterruptedException e2) {
            System.err.println(e2);
        }
        if (!j && !cls.isInterrupted() && i == 0) {
            System.out.println(0);
        }
    }
    public synchronized void run() {
        try {
            while (!Thread.currentThread().isInterrupted()) {
                wait(2000);
                break;
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            j = Thread.currentThread().isInterrupted();
            i++;
        }
    }
}