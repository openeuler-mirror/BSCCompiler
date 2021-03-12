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


public class ThreadStatewait9 extends Thread {
    static Object ob = "aa";
    static int i = 0;
    public static void main(String[] args) {
        ThreadStatewait9 threadWait = new ThreadStatewait9();
        threadWait.start();
        try {
            threadWait.join();
        } catch (InterruptedException ee) {
            System.out.println(2);
            return;
        }
        if (i == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        synchronized (ob) {
            try {
                ob.wait(-1000);
            } catch (IllegalArgumentException ee) {
                i++;
            } catch (InterruptedException e) {
                System.out.println("Wait is interrupted");
            }
        }
    }
}