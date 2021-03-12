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
public class StaticNativeNumberFormatExceptionTest extends Exception {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.exit(run(argv, System.out) + 95); /* STATUS_TEMP*/

    }
    private native void nativeStaticNativeNumberFormatExceptionTest() throws IllegalStateException; // e2
    private void callback() throws NumberFormatException { // e1
        throw new NumberFormatException("JavaMethodThrows");
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = staticNativeNumberFormatExceptionTest();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 1 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     *  Asynchronous: Static Binding: Native(exception at callback from Java), catch by Java.
     * @return status code
    */

    public static int staticNativeNumberFormatExceptionTest() {
        int result1 = 4; /* STATUS_FAILED*/

        StaticNativeNumberFormatExceptionTest cTest = new StaticNativeNumberFormatExceptionTest();
        try {
            cTest.nativeStaticNativeNumberFormatExceptionTest();
            processResult -= 10;
        } catch (NumberFormatException e1) {
            result1 = 1;
            processResult--;
        } catch (IllegalStateException e2) {
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e3) {
            processResult -= 10;
        }
        processResult -= 3;
        return result1;
    }
    static {
        try {
            System.loadLibrary("JniStaticNativeNumberFormatExceptionTest");
        } catch (UnsatisfiedLinkError e) {
            System.out.println("======>load:UnsatisfiedLinkError"); // Export LD_LIBRARY_PATH=/data/local/tmp.
        }
    }
}
