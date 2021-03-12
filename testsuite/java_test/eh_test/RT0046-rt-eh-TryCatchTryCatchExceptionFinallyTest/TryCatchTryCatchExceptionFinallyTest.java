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
import java.util.FormatterClosedException;
public class TryCatchTryCatchExceptionFinallyTest {
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
            result = tryCatchTryCatchExceptionFinallyTest1();
        } catch (NumberFormatException e) {
            processResult -= 10;
        }
        if (result == 1 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Test try{try{exception} catch(x)}-catch(e)-finally.
     * @return status code
    */

    public static int tryCatchTryCatchExceptionFinallyTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            try {
                Integer.parseInt(str);
            } catch (IllegalStateException e113) {
                System.out.println("=====See:ERROR!!!");
                processResult -= 10;
            }
            processResult -= 10;
        } catch (FormatterClosedException e1132) {
            System.out.println("=====See:ERROR!!!");
            result1 = 3;
        } catch (IllegalArgumentException e111) {
            result1 = 1;
        } catch (IndexOutOfBoundsException e112) {
            System.out.println("=====See:ERROR!!!");
            result1 = 3;
        } finally {
            processResult -= 1;
        }
        processResult -= 3;
        return result1;
    }
}
