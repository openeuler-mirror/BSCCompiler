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


public class ThreadStateJoin10Test extends Thread {
    public void run() {
        synchronized (currentThread()) {
            for (int i = 1; i <= 10000; i++) {
                try {
                    sleep(0);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
    public static void main(String[] args) {
        long startTime;
        long endTime;
        ThreadStateJoin10Test cls = new ThreadStateJoin10Test();
        cls.start();
        startTime = System.currentTimeMillis();
        try {
            cls.join(600);
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        endTime = System.currentTimeMillis();
        if (endTime-startTime >= 0 && endTime-startTime < 30) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
        return;
    }
}
