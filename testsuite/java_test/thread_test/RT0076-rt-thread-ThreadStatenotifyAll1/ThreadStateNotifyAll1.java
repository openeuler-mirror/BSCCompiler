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


class Service2 {
    static int i = 0;
    static int t = 0;
    public void testMethod(Object lock) {
        try {
            synchronized (lock) {
                lock.wait();
                i++;
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    public void synNotifyMethod(Object lock) {
        try {
            Thread.sleep(100);
            synchronized (lock) {
                lock.notifyAll();
                t++;
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
class ThreadB extends Thread {
    private Object lock;
    public ThreadB(Object lock) {
        this.lock = lock;
    }
    public void run() {
        Service2 service = new Service2();
        service.testMethod(lock);
    }
}
class SynNotifyMethodThreadB extends Thread {
    private Object lock;
    public SynNotifyMethodThreadB(Object lock) {
        this.lock = lock;
    }
    public void run() {
        Service2 service = new Service2();
        service.synNotifyMethod(lock);
    }
}
public class ThreadStateNotifyAll1 {
    public static void main(String[] args) {
        Object lock = new Object();
        ThreadB threadB = new ThreadB(lock);
        ThreadB threadB2 = new ThreadB(lock);
        SynNotifyMethodThreadB synNotifyMethodThreadB = new SynNotifyMethodThreadB(lock);
        try {
            threadB.start();
            threadB2.start();
            synNotifyMethodThreadB.start();
            Thread.sleep(1000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (!threadB.getState().equals(Thread.State.WAITING)) {
            if (!threadB2.getState().equals(Thread.State.WAITING) && Service2.i == 2 && Service2.t == 1) {
                System.out.println(0);
            }
        }
    }
}