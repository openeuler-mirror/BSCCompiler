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


public class ThreadSetDaemon1 extends Thread {
    static int i = 0;
    static int t = 0;
    public static void main(String[] args) {
        ThreadSetDaemon1 threadSetDaemon1 = new ThreadSetDaemon1();
        threadSetDaemon1.setDaemon(true);
        threadSetDaemon1.start();
        try {
            sleep(100);
            t++;
        } catch (InterruptedException e2) {
            System.out.println("Sleep is interrupted");
        }
        System.out.println(0);
    }
    public void run() {
        try {
            sleep(1000);
            i++;
            System.out.println(2);
        } catch (InterruptedException e1) {
            System.out.println("Sleep is interrupted");
        }
    }
}
