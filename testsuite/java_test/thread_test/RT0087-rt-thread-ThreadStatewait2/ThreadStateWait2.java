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


public class ThreadStateWait2 extends Thread {
    static Object object = "aa";
    static int i = 0;
    static int t = 0;
    public static void main(String[] args) {
        ThreadStateWait2 threadStateWait21 = new ThreadStateWait2();
        ThreadStateWait2 threadStateWait22 = new ThreadStateWait2();
        threadStateWait21.start();
        threadStateWait22.start();
        try {
            sleep(100);
        } catch (InterruptedException e1) {
        }
        if (threadStateWait21.getState().toString().equals("WAITING")
                && threadStateWait22.getState().toString().equals("WAITING") && i == 2 && t == 0) {
            threadStateWait21.interrupt();
            threadStateWait22.interrupt();
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (object) {
            try {
                i++;
                object.wait();
                t++;
            } catch (InterruptedException e) {
            }
        }
    }
}