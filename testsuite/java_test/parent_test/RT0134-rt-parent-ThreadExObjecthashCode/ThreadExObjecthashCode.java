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


import java.lang.Thread;
public class ThreadExObjecthashCode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new ThreadExObjecthashCode().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = threadExObjecthashCode1();
        } catch (Exception e) {
            ThreadExObjecthashCode.res = ThreadExObjecthashCode.res - 20;
        }
        if (result == 4 && ThreadExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private int threadExObjecthashCode1() {
        int result1 = 4; /*STATUS_FAILED*/
        // int hashCode()
        Thread thr1 = new Thread();
        Thread thr2 = thr1;
        Thread thr3 = new Thread();
        thr1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        thr2.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        thr3.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        if (thr1.hashCode() == thr2.hashCode() && thr1.hashCode() != thr3.hashCode()) {
            ThreadExObjecthashCode.res = ThreadExObjecthashCode.res - 10;
        } else {
            ThreadExObjecthashCode.res = ThreadExObjecthashCode.res - 5;
        }
        return result1;
    }
}