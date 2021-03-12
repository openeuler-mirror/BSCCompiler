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


public class ThreadHoldsLock1 extends Thread {
    static Object object = "aa";
    static int i = 0;
    static int t = 0;
    public static void main(String[] args) {
        ThreadHoldsLock1 threadHoldsLock1 = new ThreadHoldsLock1();
        ThreadHoldsLock1 threadHoldsLock2 = new ThreadHoldsLock1();
        threadHoldsLock1.start();
        try {
            sleep(200);
        } catch (InterruptedException e1) {
            System.out.println("Sleep is interrupted");
        }
        threadHoldsLock2.start();
        if (i == 1 && t == 1) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
    public void run() {
        synchronized (object) {
            try {
                if (holdsLock(object)) {
                    t = 1;
                }
                object.wait(100);
            } catch (InterruptedException e1) {
                System.out.println("Wait is interrupted");
            }
            if (holdsLock(object)) {
                i = 1;
            }
        }
    }
}