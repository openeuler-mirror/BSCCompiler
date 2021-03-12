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


public class ThreadConstructors15 extends Thread {
    static int i = 0;
    static int ecount = 0;
    public ThreadConstructors15(ThreadGroup group, Runnable target, String name) {
        super(group, target, name);
    }
    public static void main(String[] args) {
        try {
            ThreadConstructors15 test_illegal1 = new ThreadConstructors15(null, null, null);
        } catch (NullPointerException e) {
            //System.out.println("NullPointerException");
            ecount++;
        }
        ThreadConstructors15 test_illegal2 = new ThreadConstructors15(null, null, "");
        ThreadConstructors15 test_illegal3 = new ThreadConstructors15(null, null, "");
        test_illegal2.start();
        try {
            test_illegal2.join();
        } catch (InterruptedException e) {
            //System.out.println("NullPointerException");
        }
        test_illegal3.start();
        try {
            test_illegal3.join();
        } catch (InterruptedException e) {
            //System.out.println("NullPointerException");
        }
        if (i == 2) {
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
