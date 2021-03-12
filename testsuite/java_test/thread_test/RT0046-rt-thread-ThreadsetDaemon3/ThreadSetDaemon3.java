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


public class ThreadSetDaemon3 extends Thread {
    static int i = 0;
    static int t = 0;
    public static void main(String[] args) {
        ThreadSetDaemon3 threadSetDaemon31 = new ThreadSetDaemon3();
        ThreadSetDaemon3 threadSetDaemon32 = new ThreadSetDaemon3();
        threadSetDaemon31.setDaemon(true);
        threadSetDaemon31.start();
        try {
            sleep(50);
            t++;
        } catch (InterruptedException e3) {
            System.out.println("Sleep is interrupted");
        }
        threadSetDaemon32.start();
        try {
            sleep(50);
            t++;
        } catch (InterruptedException e2) {
            System.out.println("Sleep is interrupted");
        }
        System.out.println(0);
    }
    public void run() {
        try {
            sleep(300);
            i++;
        } catch (InterruptedException e1) {
            System.out.println("Sleep is interrupted");
        }
    }
}
