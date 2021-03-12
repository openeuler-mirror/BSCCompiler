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


class ThreadConstructors2_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors2 extends Thread {
    static int i = 0;
    public ThreadConstructors2(Runnable target) {
        super(target);
    }
    public static void main(String[] args) {
        ThreadConstructors2_a threadConstructors2_a = new ThreadConstructors2_a();
        ThreadConstructors2 threadConstructors2 = new ThreadConstructors2(threadConstructors2_a);
        threadConstructors2.start();
        try {
            threadConstructors2.join();
        } catch (InterruptedException e) {
        }
        if (i == 1) {
            if (ThreadConstructors2_a.t == 1) {
                System.out.println(0);
                return;
            }
        }
        System.out.println(2);
    }
    public void run() {
        i++;
        super.run();
    }
}