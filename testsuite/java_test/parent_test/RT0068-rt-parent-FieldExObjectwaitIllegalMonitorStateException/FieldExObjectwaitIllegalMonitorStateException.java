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


import java.lang.reflect.Field;
public class FieldExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    private static Field[] fields = FieldExObjectwaitIllegalMonitorStateException.class.getDeclaredFields();
    public static void main(String argv[]) {
        System.out.println(run());
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        // final void wait()
        try {
            result = fieldExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = fieldExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = fieldExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 20;
        }
        if (result == 4 && FieldExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }
        return result;
    }
    private static int fieldExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait()
        try {
            fields[0].wait();
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int fieldExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis)
        long millis = 123;
        try {
            fields[0].wait(millis);
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int fieldExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        long millis = 123;
        int nanos = 10;
        try {
            fields[0].wait(millis, nanos);
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            FieldExObjectwaitIllegalMonitorStateException.res = FieldExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
}