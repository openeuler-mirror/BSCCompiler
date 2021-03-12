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


class ThreadConstructors4_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors4 extends Thread {
    static int i = 0;
    public ThreadConstructors4(ThreadGroup group, Runnable target) {
        super(group, target);
    }
    public static void main(String[] args) {
        String message;
        ThreadGroup threadGroup = new ThreadGroup("god");
        ThreadConstructors4_a threadConstructors4_a = new ThreadConstructors4_a();
        ThreadConstructors4 threadConstructors4 = new ThreadConstructors4(threadGroup, threadConstructors4_a);
        message = threadConstructors4.getThreadGroup().toString();
        threadConstructors4.start();
        try {
            threadConstructors4.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (i == 1) {
            if (ThreadConstructors4_a.t == 1) {
                if (message.equals("java.lang.ThreadGroup[name=god,maxpri=10]")) {
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