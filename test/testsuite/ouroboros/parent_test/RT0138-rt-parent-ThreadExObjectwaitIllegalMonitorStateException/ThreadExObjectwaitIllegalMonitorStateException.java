/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -@TestCaseID: ThreadExObjectwaitIllegalMonitorStateException.java
 * -@TestCaseName: Exception in Thread: final void wait(*)
 * -@TestCaseType: Function Test
 * -@RequirementName: 补充重写类的父类方法
 * -@Brief:
 * -#step1: Create three methods, which call wait (), wait (millis), wait (millis, nanos)
 * -#step2: Call the run of the use case, and execute these three methods respectively
 * -#step3: Confirm that all methods throw IllegalMonitorStateException
 * -@Expect:0\n
 * -@Priority: High
 * -@Source: ThreadExObjectwaitIllegalMonitorStateException.java
 * -@ExecuteClass: ThreadExObjectwaitIllegalMonitorStateException
 * -@ExecuteArgs:
 */

import java.lang.Thread;

public class ThreadExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    static Thread mf2 = new Thread();

    public static void main(String argv[]) {
        System.out.println(run());
    }

    /**
     * main test fun
     * @return status code
     */
    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        mf2.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        // final void wait()
        try {
            result = threadExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = threadExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = threadExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 20;
        }

        if (result == 4 && ThreadExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }

        return result;
    }

    private static int threadExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait()
        try {
            mf2.wait();
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 1;
        }

        return result1;
    }

    private static int threadExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis)
        long millis = 123;
        try {
            mf2.wait(millis);
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 1;
        }

        return result1;
    }

    private static int threadExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        long millis = 123;
        int nanos = 10;
        try {
            mf2.wait(millis, nanos);
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ThreadExObjectwaitIllegalMonitorStateException.res = ThreadExObjectwaitIllegalMonitorStateException.res - 1;
        }

        return result1;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n