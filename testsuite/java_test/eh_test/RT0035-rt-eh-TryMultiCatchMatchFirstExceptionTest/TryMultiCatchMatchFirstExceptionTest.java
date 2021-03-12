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
public class TryMultiCatchMatchFirstExceptionTest {
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        result = tryMultiCatchMatchFirstExceptionTest1();
        if (result != 0) {
            return 2;
        }
        return result;
    }
    /**
     * Test try-catch/catch/...(multi-catch) mode.
     * @return status code
    */

    public static int tryMultiCatchMatchFirstExceptionTest1() {
        int result1 = 2; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            Integer.parseInt(str);
        } catch (NumberFormatException e) {
            result1 = 0;
        } catch (IllegalArgumentException e) {
            result1 = 2;
        } catch (RuntimeException e) {
            result1 = 2;
        } catch (Exception e) {
            result1 = 2;
        }
        return result1;
    }
}
