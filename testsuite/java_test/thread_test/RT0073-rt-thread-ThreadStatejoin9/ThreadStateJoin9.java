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


public class ThreadStateJoin9 extends Thread {
    static int t = 0;
    public static void main(String[] args) {
        ThreadStateJoin9 threadStateJoin9 = new ThreadStateJoin9();
        threadStateJoin9.start();
        try {
            threadStateJoin9.join(500, 300);
            t++;
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        if (threadStateJoin9.getState().toString().equals("TIMED_WAITING") && t == 3) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        for (int i = 1; i <= 5; i++) {
            try {
                sleep(200);
                t++;
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}