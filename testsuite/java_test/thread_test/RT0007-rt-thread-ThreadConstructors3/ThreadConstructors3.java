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


class ThreadConstructors3_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors3 extends Thread {
    static int i = 0;
    public ThreadConstructors3(Runnable target) {
        super(target);
    }
    public static void main(String[] args) {
        ThreadConstructors3_a threadConstructors3_a = new ThreadConstructors3_a();
        ThreadConstructors3 threadConstructors3 = new ThreadConstructors3(threadConstructors3_a);
        threadConstructors3.start();
        try {
            threadConstructors3.join();
        } catch (InterruptedException e) {
            System.out.println("Join is interrupted");
        }
        if (i == 1) {
            if (ThreadConstructors3_a.t == 0) {
                System.out.println(0);
                return;
            }
        }
        System.out.println(2);
    }
    public void run() {
        i++;
    }
}