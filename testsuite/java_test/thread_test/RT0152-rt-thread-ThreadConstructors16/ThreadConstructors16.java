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


public class ThreadConstructors16 extends Thread {
    static int i = 0;
    static int ecount = 0;
    public ThreadConstructors16(ThreadGroup group, Runnable target, String name, long stackSize) {
        super(group, target, name, stackSize);
    }
    public static void main(String[] args) {
        try {
            ThreadConstructors16 test_illegal1 = new ThreadConstructors16(null, null, null, 0);
        } catch (NullPointerException e) {
            //System.out.println("NullPointerException");
            ecount++;
        }
        try {
            ThreadConstructors16 test_illegal2 = new ThreadConstructors16(null, null, "", Long.MAX_VALUE);
        } catch (OutOfMemoryError ee) {
            System.out.println("OutOfMemoryError");
            ecount += 2;
        }
        ThreadConstructors16[] test_illegal = new ThreadConstructors16[3];
        test_illegal[0] = new ThreadConstructors16(null, null, "", 100); //Long.MIN_VALUE);
        test_illegal[1] = new ThreadConstructors16(null, null, "", new Integer("-1"));
        test_illegal[2] = new ThreadConstructors16(null, null, "", new Short("00356"));
        for (ThreadConstructors16 thread : test_illegal) {
            thread.start();
            try {
                thread.join();
            } catch (InterruptedException e) {
               // System.out.println("InterruptedException");
            }
        }
        if (i == 3) {
            if (ecount == 1) {
                System.out.println("0");
                return;
            }
        }
        System.out.println("2");
        return;
    }
    public void run() {
        i++;
        super.run();
    }
}
