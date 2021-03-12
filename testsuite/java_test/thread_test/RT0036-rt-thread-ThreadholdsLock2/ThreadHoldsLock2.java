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


public class ThreadHoldsLock2 extends Thread {
    static Object object = "aa";
    public static void main(String[] args) {
        ThreadHoldsLock2 threadHoldsLock21 = new ThreadHoldsLock2();
        ThreadHoldsLock2 threadHoldsLock22 = new ThreadHoldsLock2();
        threadHoldsLock21.start();
        try {
            sleep(200);
        } catch (InterruptedException e1) {
            System.out.println("Sleep is interrupted");
        }
        threadHoldsLock22.start();
        if ((holdsLock(object))) {
            System.out.println(2);
            return;
        }
        System.out.println(0);
    }
    public void run() {
        synchronized (object) {
            try {
                object.wait(100);
            } catch (InterruptedException e1) {
                System.out.println("Wait is interrupted");
            }
        }
    }
}