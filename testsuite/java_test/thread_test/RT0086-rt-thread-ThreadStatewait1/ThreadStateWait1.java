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


public class ThreadStateWait1 extends Thread {
    static Object object = "aa";
    static int i = 0;
    static int t = 0;
    public static void main(String[] args) {
        ThreadStateWait1 threadStateWait1 = new ThreadStateWait1();
        ThreadStateWait1 threadStateWait2 = new ThreadStateWait1();
        threadStateWait1.start();
        threadStateWait2.start();
        try {
            sleep(100);
        } catch (InterruptedException e1) {
        }
        if (threadStateWait1.getState().toString().equals("TIMED_WAITING")
                && threadStateWait2.getState().toString().equals("TIMED_WAITING") && i == 2 && t == 0) {
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (object) {
            try {
                i++;
                object.wait(1000);
                t++;
            } catch (InterruptedException e) {
            }
        }
    }
}