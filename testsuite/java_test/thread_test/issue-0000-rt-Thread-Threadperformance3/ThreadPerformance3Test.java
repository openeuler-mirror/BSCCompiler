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


public class ThreadPerformance3Test extends Thread {
    public static void main(String[] args) {
        long startTime;
        long endTime;
        Object obj = "aa";
        long sum = 0;
        long ave;
        long[] ThreadPerformance3Test = new long[20];
        Thread[] tt = new Thread[5000];
        for (int ii = 0; ii < ThreadPerformance3Test.length; ii++) {
            for (int i = 0; i < tt.length; i++) {
                tt[i] = new Thread() {
                    public void run() {
                        synchronized (obj) {
                            try {
                                obj.wait();
                            } catch (InterruptedException e1) {
                            }
                        }
                    }
                };
            }
            startTime = System.currentTimeMillis();
            for (int i = 0; i < tt.length; i++) {
                tt[i].start();
            }
            for (int i = 0; i < tt.length; i++) {
                tt[i].interrupt();
            }
            for (int i = 0; i < tt.length; i++) {
                try {
                    tt[i].join();
                } catch (InterruptedException e2) {
                    System.err.println(e2);
                }
            }
            endTime = System.currentTimeMillis();
            ThreadPerformance3Test[ii] = endTime - startTime;
        }
        for(int ii = 0; ii < ThreadPerformance3Test.length; ii++) {
            sum += ThreadPerformance3Test[ii];
        }
        ave = sum / ThreadPerformance3Test.length;
        if(ave < 6000) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
        return;
    }
}
