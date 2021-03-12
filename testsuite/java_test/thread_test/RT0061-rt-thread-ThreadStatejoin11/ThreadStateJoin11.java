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


public class ThreadStateJoin11 extends Thread {
    static Object ob = "aa";
    public static void main(String[] args) {
        long startTime;
        long endTime;
        ThreadStateJoin11 cls = new ThreadStateJoin11();
        cls.start();
        startTime = System.currentTimeMillis();
        try {
            cls.join(1000);
        } catch (InterruptedException ex) {
            System.out.println("Join is interrupted");
        }
        endTime = System.currentTimeMillis();
        if (endTime - startTime > 970 && endTime - startTime < 1030) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        synchronized (ob) {
            try {
                sleep(2000);
            } catch (InterruptedException e) {
                System.out.println("Sleep is interrupted");
            }
        }
    }
}