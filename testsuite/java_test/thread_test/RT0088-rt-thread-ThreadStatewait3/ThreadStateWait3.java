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


public class ThreadStateWait3 extends Thread {
    static Object object = "aa";
    static int i = 0;
    static int t = 0;
    public static void main(String[] args) {
        ThreadStateWait3 threadStateWait31 = new ThreadStateWait3();
        ThreadStateWait3 threadStateWait32 = new ThreadStateWait3();
        threadStateWait31.start();
        threadStateWait32.start();
        try {
            sleep(100);
        } catch (InterruptedException e1) {
        }
        if (threadStateWait31.getState().toString().equals("TIMED_WAITING")
                && threadStateWait32.getState().toString().equals("TIMED_WAITING") && i == 2 && t == 0) {
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (object) {
            try {
                i++;
                object.wait(1000, 500);
                t++;
            } catch (InterruptedException e) {
            }
        }
    }
}