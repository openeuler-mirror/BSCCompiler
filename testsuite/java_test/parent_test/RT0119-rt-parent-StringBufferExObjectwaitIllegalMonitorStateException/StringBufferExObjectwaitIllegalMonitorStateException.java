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


import java.lang.StringBuffer;
public class StringBufferExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    static private String[] stringArray = {
            "", "a", "b", "c", "ab", "ac", "abc", "aaaabbbccc"
    };
    static private StringBuffer mf2 = null;
    public static void main(String argv[]) {
        for (int i = 0; i < stringArray.length; i++) {
            mf2 = new StringBuffer(stringArray[i]);
        }
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
            result = stringBufferExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = stringBufferExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = stringBufferExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 20;
        }
        if (result == 4 && StringBufferExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }
        return result;
    }
    private static int stringBufferExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait()
        try {
            mf2.wait();
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int stringBufferExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis)
        long millis = 123;
        try {
            mf2.wait(millis);
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int stringBufferExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        long millis = 123;
        int nanos = 10;
        try {
            mf2.wait(millis, nanos);
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            StringBufferExObjectwaitIllegalMonitorStateException.res = StringBufferExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
}