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


public class ThreadStateJoin15 extends Thread {
    public static void main(String[] args) {
        ThreadStateJoin15 cls = new ThreadStateJoin15();
        cls.start();
        try {
            cls.join(1000, -10);
        } catch (InterruptedException e1) {
            System.out.println(2);
        } catch (IllegalArgumentException e2) {
            System.out.println(0);
        }
    }
    public void run() {
        synchronized (currentThread()) {
            for (int i = 1; i <= 3; i++) {
                try {
                    sleep(200);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}