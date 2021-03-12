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


public class ThreadCurrentThread extends Thread {
    static int nameMatch = 0;
    static Object a = new Object();
    public static void main(String[] args) {
        ThreadCurrentThread threadCurrentThread1 = new ThreadCurrentThread();
        ThreadCurrentThread threadCurrentThread2 = new ThreadCurrentThread();
        threadCurrentThread1.setName("good");
        threadCurrentThread2.setName("bad");
        threadCurrentThread1.start();
        threadCurrentThread2.start();
        try {
            threadCurrentThread1.join();
            threadCurrentThread2.join();
        } catch (Exception e) {
            System.out.println("Join is interrupted");
        }
        if (currentThread().getName().equals("good")) {
            System.out.println(2);
        }
        if (currentThread().getName().equals("bad")) {
            System.out.println(3);
        }
        if (currentThread().getName().equals("main")) {
            if (nameMatch == 2) {
                System.out.println(0);
                return;
            }
        }
        System.out.println(2);
    }
    public synchronized void run() {
        if (currentThread().getName() == getName()) {
            synchronized (a) {
                nameMatch++;
            }
        }
        try {
            wait(500);
        } catch (InterruptedException e1) {
            System.out.println("Wait is interrupted");
        }
    }
}
