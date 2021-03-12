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


import java.lang.reflect.AccessibleObject;
public class AccessibleObjectExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    private static AccessibleObject sampleField1 = null;
    public static void main(String argv[]) throws NoSuchFieldException, SecurityException {
        sampleField1 = AccessibleObjectExObjectwaitIllegalMonitorStateException.class.getDeclaredField("res");
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
            result = accessibleObjectExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = accessibleObjectExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = accessibleObjectExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 20;
        }
        if (result == 4 && AccessibleObjectExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }
        return result;
    }
    private static int accessibleObjectExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait()
        try {
            sampleField1.wait();
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int accessibleObjectExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        //
        // final void wait(long millis)
        long millis = 123;
        try {
            sampleField1.wait(millis);
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int accessibleObjectExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        long millis = 123;
        int nanos = 10;
        try {
            sampleField1.wait(millis, nanos);
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            AccessibleObjectExObjectwaitIllegalMonitorStateException.res = AccessibleObjectExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
}