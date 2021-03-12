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


public class ThreadConstructors11 extends Thread {
    static int i = 0;
    public ThreadConstructors11(Runnable target) {
        super(target);
    }
    public static void main(String[] args) {
        ThreadConstructors11 test_illegal1 = new ThreadConstructors11(null);
        test_illegal1.start();
        try {
            test_illegal1.join();
        } catch (InterruptedException e) {
            System.out.println("Join is interrupted");
        }
        if (i == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        i++;
    }
}