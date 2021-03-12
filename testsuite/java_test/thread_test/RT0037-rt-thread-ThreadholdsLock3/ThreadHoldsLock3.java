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


public class ThreadHoldsLock3 extends Thread {
    static Object object = "aa";
    static Object object1 = null;
    public static void main(String[] args) {
        ThreadHoldsLock3 threadHoldsLock31 = new ThreadHoldsLock3();
        ThreadHoldsLock3 threadHoldsLock32 = new ThreadHoldsLock3();
        threadHoldsLock31.start();
        try {
            sleep(200);
        } catch (InterruptedException e1) {
            System.out.println("Sleep is interrupted");
        }
        threadHoldsLock32.start();
        try {
            holdsLock(object1);
            System.out.println(2);
        } catch (NullPointerException e) {
            System.out.println(0);
        }
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