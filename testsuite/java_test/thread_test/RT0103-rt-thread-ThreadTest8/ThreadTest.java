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


class ThreadRunning1 extends Thread {
    public volatile int i = 0;
    volatile boolean stopWork = false;
    Object mylock;
    ThreadRunning1(Object obj) {
        mylock = obj;
    }
    public void run() {
        synchronized (mylock) {
            while (!stopWork) {
                i++;
                i--;
            }
        }
    }
}
class ThreadRunning2 extends Thread {
    public volatile int i = 0;
    volatile boolean stopWork = false;
    public void run() {
        try {
            Thread.sleep(1000);
        } catch (Exception e) {
        }
    }
}
class ThreadRunning3 extends Thread {
    public volatile int i = 0;
    volatile boolean stopWork = false;
    Object mylock;
    ThreadRunning3(Object obj) {
        mylock = obj;
    }
    public void run() {
        synchronized (mylock) {
            try {
                mylock.wait();
                while (!stopWork) {
                    i++;
                    i--;
                }
            } catch (Exception e) {
            }
        }
    }
}
public class ThreadTest {
    public static void main(String[] args) throws Exception {
        Object object = new Object();
        ThreadRunning1 threadRunning1 = new ThreadRunning1(object);
        ThreadRunning2 threadRunning2 = new ThreadRunning2();
        ThreadRunning3 threadRunning3 = new ThreadRunning3(object);
        // t1 NEW; t2 NEW; t3 NEW
        System.out.println(threadRunning1.getState());
        threadRunning1.start();
        threadRunning2.start();
        // t1 RUNNABLE; t2 TIMED_WAITING; t3 NEW
        System.out.println(threadRunning1.getState());
        try {
            Thread.sleep(200);
        } catch (Exception e) {
        }
        threadRunning3.start();
        try {
            Thread.sleep(200);
        } catch (Exception e) {
        }
        // t1 RUNNABLE; t2 TIMED_WAITING; t3 BmymylockED
        System.out.println(threadRunning3.getState());
        threadRunning1.stopWork = true;
        try {
            Thread.sleep(100);
        } catch (Exception e) {
        }
        // t1 TERMINATED; t2 TIMED_WAITING; t3 WAITING
        System.out.println(threadRunning3.getState());
        System.out.println(threadRunning2.getState());
        System.out.println(threadRunning1.getState());
        threadRunning3.stopWork = true;
        synchronized (object) {
            object.notify();
        }
    }
}