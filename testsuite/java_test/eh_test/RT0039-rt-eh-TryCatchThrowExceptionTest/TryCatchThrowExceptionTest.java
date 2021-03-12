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
public class TryCatchThrowExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = tryCatchThrowExceptionTest1();
        } catch (NumberFormatException e) {
            processResult--;
        }
        if (processResult == 97) {
            result--;
        }
        try {
            tryCatchThrowExceptionTest3();
        } catch (Exception3 e) {
            processResult--;
        }
        if (processResult == 95) {
            result--;
        }
        return result;
    }
    /**
     * Test static int parseInt(String s), NumberFormatException is caught by catch
     * @return status code
     * @throws NumberFormatException
    */

    public static int tryCatchThrowExceptionTest1() throws NumberFormatException {
        int result1 = 4; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            Integer.parseInt(str);
        } catch (NumberFormatException e) {
            processResult--;
            throw e;
        }
        processResult -= 10; // Compiled successfully, but cannot execute.
        return result1;
    }
    /**
     * Create a new Exception extends from Exception.
     * @return status code
     * @throws Exception3
    */

    public static int tryCatchThrowExceptionTest3() throws Exception3 {
        int result3 = 3; /* STATUS_FAILED*/

        if (result3 != 4) {
            processResult--;
            throw new Exception3();
        }
        processResult -= 10; // Compiled successfully, but cannot execute.
        return result3;
    }
    static class Exception3 extends Exception {
        Exception3() {
            super("yes");
        }
    }
}
