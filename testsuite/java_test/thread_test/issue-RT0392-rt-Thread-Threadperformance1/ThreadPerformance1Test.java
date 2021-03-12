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


class LockCls {}
public class ThreadPerformance1Test extends  Thread{
    public static void main(String[] args) {
        LockCls lock = new LockCls();
        long startTime;
        long endTime;
        long[] ThreadPerformance1Test = new long[20];
        long sum = 0;
        long ave;
        Object obj = "aa";
        Thread[] tt = new Thread[100];
        for (int ii = 0; ii < ThreadPerformance1Test.length; ii++) {
            for (int i = 0; i < tt.length; i++) {
                tt[i] = new Thread(new Runnable() {
                    public void run() {
                        synchronized (lock) {
                            synchronized (lock) {
                                synchronized (obj) {
                                    synchronized (this) {
                                        synchronized (obj) {
                                            synchronized (this) {
                                                try {
                                                    this.wait(10);
                                                } catch (InterruptedException e1) {
                                                    System.err.println(e1);
                                                }
                                            }
                                            try {
                                                obj.wait(10);
                                            } catch (InterruptedException e2) {
                                                System.err.println(e2);
                                            }
                                        }
                                        this.notifyAll();
                                    }
                                    obj.notify();
                                }
                                try {
                                    lock.wait(10);
                                } catch (InterruptedException e3) {
                                    System.err.println(e3);
                                }
                            }
                            lock.notifyAll();
                        }
                    }
                });
            }
            startTime = System.currentTimeMillis();
            for (int i = 0; i < tt.length; i++) {
                tt[i].start();
            }
            for (int i = 0; i < tt.length; i++) {
                try {
                    tt[i].join();
                }catch (InterruptedException e) {
                    System.err.println(e);
                }
            }
            endTime = System.currentTimeMillis();
            //System.out.println(startTime);
            //System.out.println(endTime);
            //System.out.println(endTime - startTime);
            ThreadPerformance1Test[ii] = endTime - startTime;
        }
        for(int ii = 0; ii < ThreadPerformance1Test.length; ii++) {
            sum += ThreadPerformance1Test[ii];
        }
        ave = sum / ThreadPerformance1Test.length;
        //System.out.println("ave=" + ave + "ms");
        System.out.println(0);
        return;
    }
}
