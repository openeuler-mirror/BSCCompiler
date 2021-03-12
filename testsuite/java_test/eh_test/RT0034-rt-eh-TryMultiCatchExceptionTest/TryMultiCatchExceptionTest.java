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
import java.nio.file.InvalidPathException;
public class TryMultiCatchExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2;
        try {
            tryMultiCatchExceptionTest1();
            tryMultiCatchExceptionTest2();
            tryMultiCatchExceptionTest3();
            tryMultiCatchExceptionTest4();
            tryMultiCatchExceptionTest5();
        } catch (Exception e) {
            processResult -= 10;
            result = 3;
        }
        if (result == 2 && processResult == 94) {
            result = 0;
        }
        return result;
    }
    /**
     * Use try-catch/catch/...(multi-catch) test static int parseInt(String s), NumberFormatException is in first catch.
    */

    public static void tryMultiCatchExceptionTest1() {
        String str = "123#456";
        try {
            Integer.parseInt(str);
            processResult -= 10;
        } catch (NumberFormatException e) {
            processResult--;
        } catch (ClassCastException e) {
            processResult -= 10;
        } catch (InvalidPathException e) {
            processResult -= 10;
        } catch (IllegalStateException e) {
            processResult -= 10;
        } catch (IllegalArgumentException e) {
            processResult -= 10;
        }
    }
    /**
     * Use try-catch/catch/...(multi-catch) test static int parseInt(String s), NumberFormatException is in second catch.
    */

    public static void tryMultiCatchExceptionTest2() {
        String str = "123#456";
        try {
            Integer.parseInt(str);
            processResult -= 10;
        } catch (ClassCastException e) {
            processResult -= 10;
        } catch (NumberFormatException e) {
            processResult--;
        } catch (InvalidPathException e) {
            processResult -= 10;
        } catch (IllegalStateException e) {
            processResult -= 10;
        } catch (IllegalArgumentException e) {
            processResult -= 10;
        }
    }
    /**
     * Use try-catch/catch/...(multi-catch) test static int parseInt(String s), NumberFormatException is in third catch.
    */

    public static void tryMultiCatchExceptionTest3() {
        String str = "123#456";
        try {
            Integer.parseInt(str);
            processResult -= 10;
        } catch (ClassCastException e) {
            processResult -= 10;
        } catch (InvalidPathException e) {
            processResult -= 10;
        } catch (NumberFormatException e) {
            processResult--;
        } catch (IllegalStateException e) {
            processResult -= 10;
        } catch (IllegalArgumentException e) {
            processResult -= 10;
        }
    }
    /**
     * Use try-catch/catch/...(multi-catch) test static int parseInt(String s), NumberFormatException is in forth catch.
    */

    public static void tryMultiCatchExceptionTest4() {
        String str = "123#456";
        try {
            Integer.parseInt(str);
            processResult -= 10;
        } catch (ClassCastException e) {
            processResult -= 10;
        } catch (InvalidPathException e) {
            processResult -= 10;
        } catch (IllegalStateException e) {
            processResult -= 10;
        } catch (NumberFormatException e) {
            processResult--;
        } catch (IllegalArgumentException e) {
            processResult -= 10;
        }
    }
    /**
     * Use try-catch/catch/...(multi-catch) test static int parseInt(String s), NumberFormatException is in fifth catch.
    */

    public static void tryMultiCatchExceptionTest5() {
        String str = "123#456";
        try {
            Integer.parseInt(str);
            processResult -= 10;
        } catch (ClassCastException e) {
            processResult -= 10;
        } catch (InvalidPathException e) {
            processResult -= 10;
        } catch (IllegalStateException e) {
            processResult -= 10;
        } catch (IndexOutOfBoundsException e) {
            processResult -= 10;
        } catch (NumberFormatException e) {
            processResult--;
        }
    }
}
