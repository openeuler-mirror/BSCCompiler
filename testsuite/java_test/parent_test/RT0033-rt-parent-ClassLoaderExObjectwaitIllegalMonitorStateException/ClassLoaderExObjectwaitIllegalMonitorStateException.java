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
public class ClassLoaderExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    private static ClassLoader cl = ClassLoaderExObjectwaitIllegalMonitorStateException.class.getClassLoader();
    public static void main(String argv[]) {
        System.out.println(run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        // final void wait()
        try {
            result = classLoaderExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = classLoaderExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = classLoaderExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 20;
        }
        if (result == 4 && ClassLoaderExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }
        return result;
    }
    private static int classLoaderExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait()
        try {
            cl.wait();
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int classLoaderExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis)
        long millis = 123;
        try {
            cl.wait(millis);
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int classLoaderExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        long millis = 123;
        int nanos = 10;
        try {
            cl.wait(millis, nanos);
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ClassLoaderExObjectwaitIllegalMonitorStateException.res = ClassLoaderExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
}