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


public class ThreadSetDaemon2 extends Thread {
    public static void main(String[] args) {
        ThreadSetDaemon2 threadSetDaemon2 = new ThreadSetDaemon2();
        threadSetDaemon2.start();
        try {
            threadSetDaemon2.setDaemon(true);
            System.out.println(2);
        } catch (IllegalThreadStateException e2) {
            System.out.println(0);
        }
    }
    public void run() {
        for (int i = 0; i < 5; i++) {
            try {
                sleep(200);
            } catch (InterruptedException e1) {
                System.out.println("Sleep is interrupted");
            }
        }
    }
}