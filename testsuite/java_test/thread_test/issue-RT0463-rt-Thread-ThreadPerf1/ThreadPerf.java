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


public class ThreadPerf {
    static long test(final int threadCount, final int workAmountPerThread) throws InterruptedException {
        Thread[] tt = new Thread[threadCount];
        final int[] aa = new int[tt.length];
//        System.out.print("Creating " + tt.length + " Thread objects.");
        long t0 = System.nanoTime(), t00 = t0;
        for (int i = 0; i < tt.length; i++) { 
            final int j = i;
            tt[i] = new Thread() {
                public void run() {
                    int k = j;
                    for (int l = 0; l < workAmountPerThread; l++) {
                        k += k*k+l;
                    }
                    aa[j] = k;
                }
            };
        }
//        System.out.println("-- Done in " + (System.nanoTime() - t0)*1E-6 + " ms.");
//        System.out.println("Starting " + tt.length + " threads with " + workAmountPerThread + " steps of work per thread.");
        t0 = System.nanoTime();
        for (int i = 0; i < tt.length; i++) { 
            tt[i].start();
        }
        long t1 = System.nanoTime() - t0;
//        System.out.println("-- Done in " + t1 * 1E-6 + " ms.");
//        System.out.println("Joining " + tt.length + " threads.");
        t0 = System.nanoTime();
        for (int i = 0; i < tt.length; i++) { 
            tt[i].join();
        }
        long t2 = System.nanoTime() - t0;
//        System.out.println("-- Done in "+ t2*1E-6 + " ms.");
        long totalTime = t1 + t2; //total time during thread.start() and thread.join()
        int checkSum = 0; 
        //display checksum in order to give the JVM no chance 
        //to optimize out the contents of the run() method and possibly even thread creation
        for (int a : aa) {
            checkSum += a;
        }
//        System.out.println("Checksum: " + checkSum);
//        System.out.println("Total time: " + totalTime*1E-6 + " ms");
//        System.out.println();
        return totalTime;
    }
    public static void main(String[] kr) throws InterruptedException {
        int workAmount = 100000000;
        int[] threadCount = new int[]{1, 2, 10, 50, 100, 200, 500, 1000, 10000};
        int trialCount = 3; //run the test trialCount times
        long[][] time = new long[threadCount.length][trialCount];
        for (int j = 0; j < trialCount; j++) {
            for (int i = 0; i < threadCount.length; i++) {
                time[i][j] = test(threadCount[i], workAmount/threadCount[i]); 
            }
        }
//        System.out.println("***************** time during thread.start() and thread.join() ******************\n");
//        System.out.print("Number of threads ");
        for (long t : threadCount) {
//            System.out.print("\t" + t);
        }
//        System.out.println();
        for (int j = 0; j < trialCount; j++) {
//            System.out.print((j + 1) + ". trial time (ms)");
            for (int i = 0; i < threadCount.length; i++) {
//                System.out.print("\t" + Math.round(time[i][j]*1E-6));
            }
//            System.out.println();
        }
	System.out.println(0);
    }
}
/*
The results on 64-bit Windows 7 with 32-bit Sun's Java 1.6.0_21 Client VM on Intel Core2 Duo E6400 @2.13 GHz are as follows:
Number of threads  1    2    10   100  1000 10000 100000
1. trial time (ms) 346  181  179  191  286  1229  11308
2. trial time (ms) 346  181  187  189  281  1224  10651
*/
