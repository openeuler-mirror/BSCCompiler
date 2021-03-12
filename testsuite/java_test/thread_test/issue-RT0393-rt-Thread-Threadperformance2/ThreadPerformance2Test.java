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


public class ThreadPerformance2Test extends Thread {
    public static void main(String[] args) {
        long startTime;
        long endTime;
        long sum = 0;
        long ave;
        long[] ThreadPerformance2Test = new long[20];
        Thread[] tt = new Thread[10000];
        for (int ii = 0; ii < ThreadPerformance2Test.length; ii++) {
            for (int i = 0; i < tt.length; i++) {
                tt[i] = new Thread() {
                    public void run() {
                        yield();
                        try {
                            sleep(0);
                        }catch (InterruptedException e) {
                            System.err.println(e);
                        }
                    }
                };
            }
            startTime = System.currentTimeMillis();
            for (int i = 0; i < tt.length; i++) {
                tt[i].start();
            }
            for (int i = 0; i < tt.length; i++) {
                try {
                    tt[i].join();
                } catch (InterruptedException e) {
                    System.err.println(e);
                }
            }
            endTime = System.currentTimeMillis();
            ThreadPerformance2Test[ii] = endTime - startTime;
        }
        for (int ii = 0; ii < ThreadPerformance2Test.length; ii++) {
            sum += ThreadPerformance2Test[ii];
        }
        ave = sum / ThreadPerformance2Test.length;
        if (ave < 6000) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
        return;
    }
}
