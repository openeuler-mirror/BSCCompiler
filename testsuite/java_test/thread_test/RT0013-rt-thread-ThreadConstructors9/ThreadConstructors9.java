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


class ThreadConstructors9_a implements Runnable {
    static int t = 0;
    public void run() {
        t++;
    }
}
public class ThreadConstructors9 extends Thread {
    static int i = 0;
    static long k;
    public ThreadConstructors9(ThreadGroup group, Runnable target, String name, long stackSize) {
        super(group, target, name, stackSize);
        k = stackSize;
    }
    public static void main(String[] args) {
        String message;
        ThreadGroup threadGroup = new ThreadGroup("god");
        ThreadConstructors9_a threadConstructors9_a = new ThreadConstructors9_a();
        ThreadConstructors9 threadConstructors9 = new ThreadConstructors9(threadGroup, threadConstructors9_a,
                "good", 1 << 30);
        message = threadConstructors9.getThreadGroup().toString();
        threadConstructors9.start();
        try {
            threadConstructors9.join();
        } catch (InterruptedException e) {
            System.out.println("Join is interrupted");
        }
        if (i == 1 && k == 1073741824 && ThreadConstructors9_a.t == 1) {
            if (message.equals("java.lang.ThreadGroup[name=god,maxpri=10]")) {
                if (threadConstructors9.getName().equals("good")) {
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