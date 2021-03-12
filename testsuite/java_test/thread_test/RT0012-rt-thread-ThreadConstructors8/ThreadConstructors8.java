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


class ThreadConstructors8_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors8 extends Thread {
    static int i = 0;
    public ThreadConstructors8(ThreadGroup group, Runnable target, String name) {
        super(group, target, name);
    }
    public static void main(String[] args) {
        String message;
        ThreadGroup threadGroup = new ThreadGroup("god");
        ThreadConstructors8_a threadConstructors8_a = new ThreadConstructors8_a();
        ThreadConstructors8 threadConstructors8 = new ThreadConstructors8(threadGroup, threadConstructors8_a,
                "good");
        message = threadConstructors8.getThreadGroup().toString();
        threadConstructors8.start();
        try {
            threadConstructors8.join();
        } catch (InterruptedException e) {
            System.out.println("Join is interrupted");
        }
        if (i == 1 && ThreadConstructors8_a.t == 1) {
            if (message.equals("java.lang.ThreadGroup[name=god,maxpri=10]")) {
                if (threadConstructors8.getName().equals("good")) {
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