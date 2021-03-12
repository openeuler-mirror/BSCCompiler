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


public class ThreadStateJoin12 extends Thread {
    public static void main(String[] args) {
        long startTime;
        long endTime;
        ThreadStateJoin12 cls = new ThreadStateJoin12();
        cls.start();
        startTime = System.currentTimeMillis();
        try {
            cls.join(600);
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        endTime = System.currentTimeMillis();
        if (endTime - startTime > 590 && endTime - startTime < 1010) {
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (currentThread()) {
            for (int i = 1; i <= 5; i++) {
                try {
                    sleep(200);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}