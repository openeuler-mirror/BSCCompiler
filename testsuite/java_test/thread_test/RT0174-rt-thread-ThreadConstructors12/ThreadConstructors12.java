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


public class ThreadConstructors12 extends Thread {
    static int i = 0;
    static int eCount = 0;
    public ThreadConstructors12(Runnable target, String name) {
        super(target, name);
    }
    public static void main(String[] args) {
        try {
            ThreadConstructors12 test_illegal1 = new ThreadConstructors12(null, null);
        } catch (NullPointerException e) {
            eCount++;
        }
        ThreadConstructors12 test_illegal2 = new ThreadConstructors12(null, "");
        ThreadConstructors12 test_illegal3 = new ThreadConstructors12(null, new String());
        test_illegal2.start();
        try {
            test_illegal2.join();
        } catch (InterruptedException e) {
            System.out.println("test_illegal2 join is interrupted");
        }
        test_illegal3.start();
        try {
            test_illegal3.join();
        } catch (InterruptedException e) {
            System.out.println("test_illegal3 join is interrupted");
        }
        if (i == 2 && eCount == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        i++;
        super.run();
    }
}