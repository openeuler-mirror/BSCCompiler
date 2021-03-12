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


class Service1 {
    static int i = 0;
    static int t = 0;
    public void testMethod(Object lock) {
        try {
            synchronized (lock) {
                lock.wait(3000);
                i++;
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    public void syncNotifyMethod(Object lock) {
        try {
            Thread.sleep(100);
            synchronized (lock) {
                lock.notify();
                t++;
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
class ThreadA extends Thread {
    private Object lock;
    public ThreadA(Object lock) {
        this.lock = lock;
    }
    public void run() {
        Service1 service = new Service1();
        service.testMethod(lock);
    }
}
class SynNotifyMethodThreadA extends Thread {
    private Object lock;
    public SynNotifyMethodThreadA(Object lock) {
        this.lock = lock;
    }
    public void run() {
        Service1 service = new Service1();
        service.syncNotifyMethod(lock);
    }
}
public class ThreadStateNotify1 {
    public static void main(String[] args) {
        Object lock = new Object();
        ThreadA threadA = new ThreadA(lock);
        ThreadA threadA2 = new ThreadA(lock);
        SynNotifyMethodThreadA synNotifyMethodThreadA = new SynNotifyMethodThreadA(lock);
        try {
            threadA.start();
            threadA2.start();
            synNotifyMethodThreadA.start();
            Thread.sleep(1000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (!threadA.getState().equals(Thread.State.TIMED_WAITING) && Service1.i == 1 && Service1.t == 1) {
            System.out.println(0);
            return;
        }
        if (!threadA2.getState().equals(Thread.State.TIMED_WAITING) && Service1.i == 1 && Service1.t == 1) {
            System.out.println(0);
        }
    }
}