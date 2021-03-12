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


public class ThreadStateSleep2 extends Thread {
    static Object object = "aa";
    static int t = 0;
    static int b = 0;
    public static void main(String[] args) {
        ThreadStateSleep2 threadStateSleep21 = new ThreadStateSleep2();
        ThreadStateSleep2 threadStateSleep22 = new ThreadStateSleep2();
        threadStateSleep21.start();
        threadStateSleep22.start();
        try {
            sleep(100);
        } catch (InterruptedException e1) {
        }
        if (threadStateSleep21.getState().toString().equals("BLOCKED")
                && threadStateSleep22.getState().toString().equals("TIMED_WAITING") && t == 1 && b == 0) {
            System.out.println(0);
        }
        if (threadStateSleep22.getState().toString().equals("BLOCKED")
                && threadStateSleep21.getState().toString().equals("TIMED_WAITING") && t == 1 && b == 0) {
            System.out.println(0);
        }
    }
    public void run() {
        for (int i = 0; i < 1; i++) {
            synchronized (object) {
                try {
                    t++;
                    sleep(1000);
                    b++;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}