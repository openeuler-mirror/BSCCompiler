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


class ThreadConstructors10_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors10 extends Thread {
    public ThreadConstructors10(Runnable target) {
        super(target);
    }
    public static void main(String[] args) {
        ThreadConstructors10_a threadConstructors10_a = new ThreadConstructors10_a();
        ThreadConstructors10 threadConstructors10 = new ThreadConstructors10(threadConstructors10_a);
        threadConstructors10.start();
        try {
            threadConstructors10.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (ThreadConstructors10_a.t == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}