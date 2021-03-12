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


import java.io.PrintStream;
public class TryMultiCatchFinallyExceptionTest {
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2;
        result = tryMultiCatchFinallyExceptionTest();
        if (result != 0) {
            return 2;
        }
        try {
            result = tryMultiCatchFinallyExceptionTest2();
        } catch (NumberFormatException e) {
            if (result != 0) {
                return 2;
            }
        }
        return result;
    }
    /**
     * Use try-catch/catch…-finally to test static int parseInt(String s), NumberFormatException is in catch.
     * @return status code
    */

    public static int tryMultiCatchFinallyExceptionTest() {
        int result1 = 2; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            Integer.parseInt(str);
        } catch (NumberFormatException e) {
            result1--;
        } catch (IllegalArgumentException e) {
            result1 = 2;
        } catch (RuntimeException e) {
            result1 = 2;
        } catch (Exception e) {
            result1 = 2;
        } finally {
            result1--;
        }
        return result1;
    }
    /**
     * Use try-catch/catch…-finally to test static int parseInt(String s), NumberFormatException is not in catch.
     * @return status code
     * @throws NumberFormatException
    */

    public static int tryMultiCatchFinallyExceptionTest2() throws NumberFormatException {
        int result2 = 2; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            Integer.parseInt(str);
        } catch (IndexOutOfBoundsException e) {
            result2 = 2;
        } finally {
            result2 = 0;
        }
        return result2;
    }
}
