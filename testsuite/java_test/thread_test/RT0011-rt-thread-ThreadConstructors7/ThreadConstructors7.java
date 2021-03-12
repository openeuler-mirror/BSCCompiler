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


class ThreadConstructors7_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors7 extends Thread {
    static int i = 0;
    public ThreadConstructors7(Runnable target, String name) {
        super(target, name);
    }
    public static void main(String[] args) {
        ThreadConstructors7_a threadConstructors7_a = new ThreadConstructors7_a();
        ThreadConstructors7 threadConstructors7 = new ThreadConstructors7(threadConstructors7_a, "good");
        threadConstructors7.start();
        try {
            threadConstructors7.join();
        } catch (InterruptedException e) {
            System.out.println("Join is interrupted");
        }
        if (i == 1) {
            if (ThreadConstructors7_a.t == 1) {
                if (threadConstructors7.getName().equals("good")) {
                    System.out.println(0);
                    return;
                }
            }
        }
        System.out.println(2);
    }
    public void run() {
        i++;
        super.run();
    }
}