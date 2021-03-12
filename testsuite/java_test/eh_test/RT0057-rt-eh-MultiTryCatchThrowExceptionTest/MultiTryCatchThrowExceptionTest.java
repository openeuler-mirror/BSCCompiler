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
public class MultiTryCatchThrowExceptionTest {
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
            result = multiTryCatchThrowExceptionTest1();
        } catch (Exception e) {
            processResult -= 4;
        }
        if (result == 2 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Test try{try{try{try{try{exception}catch()}catch()}catch()}catch()},catch(x):-->caller.
     * @return status code
    */

    public static int multiTryCatchThrowExceptionTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            processResult += 100;
            try {
                processResult += 100;
                try {
                    processResult += 100;
                    try {
                        processResult += 100;
                        try {
                            processResult += 100;
                            Integer.parseInt(str);
                        } catch (ClassCastException e) {
                            processResult -= 10;
                        } finally {
                            processResult -= 100;
                        }
                    } catch (ClassCastException e) {
                        processResult -= 10;
                    } finally {
                        processResult -= 100;
                    }
                } catch (ClassCastException e) {
                    processResult -= 10;
                } finally {
                    processResult -= 100;
                }
            } catch (IllegalStateException e) {
                processResult -= 10;
            } finally {
                processResult -= 100;
            }
            processResult -= 10;
        } catch (IndexOutOfBoundsException e) {
            processResult -= 10;
        } catch (IllegalStateException e) {
            processResult -= 10;
        } finally {
            processResult -= 100;
        }
        processResult -= 10;
        return result1;
    }
}
