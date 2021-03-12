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


import java.io.PrintStream;
public class ThreadStateyield extends Thread {
    static int i = 0;
    public ThreadStateyield(String name) {
        super(name);
    }
    public static void main(String[] args) {
        System.exit(run(args, System.out));
    }
    public static int run(String[] args, PrintStream out) {
        ThreadStateyield zqp1 = new ThreadStateyield("zqp1");
        ThreadStateyield zqp2 = new ThreadStateyield("zqp2");
        zqp1.start();
        try {
            sleep(10);
        } catch (InterruptedException e1) {
        }
        zqp2.start();
        try {
            sleep(1000);
        } catch (InterruptedException e1) {
        }
        System.out.println(zqp2.getState());
        System.out.println(zqp1.getState());
        if (zqp2.getState().toString().equals("TERMINATED") && zqp1.getState().toString().equals("TIMED_WAITING") && i != 0) {
            return 0;
        }
        return 2;
    }
    public void run() {
        for (; ; ) {
            if (currentThread().getName().equals("zqp1")) {
                yield();
                i++;
                try {
                    sleep(5000);
                } catch (InterruptedException e) {
                }
            } else break;
        }
    }
}
