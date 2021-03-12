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


public class ThreadStateSleep4 extends Thread {
    static Object object = "aa";
    static long start = 0;
    static long end = 0;
    public static void main(String[] args) {
        ThreadStateSleep4 threadStateSleep4 = new ThreadStateSleep4();
        threadStateSleep4.start();
        try {
            threadStateSleep4.join();
        } catch (InterruptedException ex) {
            System.out.println(ex);
        }
        if (end - start > 970 && end - start < 1030) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
    public void run() {
        synchronized (object) {
            try {
                start = System.currentTimeMillis();
                sleep(1000);
                end = System.currentTimeMillis();
            } catch (InterruptedException e) {
                System.out.println(e);
            }
        }
    }
}