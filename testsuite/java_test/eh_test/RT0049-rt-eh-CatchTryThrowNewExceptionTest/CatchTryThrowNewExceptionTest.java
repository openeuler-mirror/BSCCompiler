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
public class CatchTryThrowNewExceptionTest {
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
            result = catchTryThrowNewExceptionTest1();
        } catch (StringIndexOutOfBoundsException e) {
            processResult--;
        }
        if (result == 2 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Test try-catch(){try{exception},catch(x)}-finally --> caller.
     * @return status code
    */

    public static int catchTryThrowNewExceptionTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            Integer.parseInt(str);
            System.out.println("=====See:ERROR!!!");
        } catch (ClassCastException e) {
            System.out.println("=====See:ERROR!!!");
            result1 = 3;
        } catch (NumberFormatException e) {
            result1 = 1;
            try {
                System.out.println(str.substring(-5));
            } catch (NumberFormatException e2) {
                processResult -= 10;
            }
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e) {
            System.out.println("=====See:ERROR!!!");
            result1 = 3;
        } finally {
            processResult -= 3;
        }
        processResult -= 10;
        return result1;
    }
}
