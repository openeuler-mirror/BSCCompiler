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
public class NativeExNumberFormatExceptionTest extends Exception {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.exit(run(argv, System.out) + 95); /* STATUS_TEMP*/

    }
    private native void nativeNativeExNumberFormatExceptionTest() throws InvalidPathException; // e2
    private void callback() throws NumberFormatException { // e1
        throw new NumberFormatException("JavaMethodThrows");
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = nativeExNumberFormatExceptionTest1();
        } catch (Exception e) {
            NativeExNumberFormatExceptionTest.processResult = NativeExNumberFormatExceptionTest.processResult - 10;
        }
        if (result == 3 && NativeExNumberFormatExceptionTest.processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * main test fun Entrance
     * @return status code
    */

    public static int nativeExNumberFormatExceptionTest1() {
        int result1 = 4; /*STATUS_FAILED*/
        NativeExNumberFormatExceptionTest cTest = new NativeExNumberFormatExceptionTest();
        try {
            cTest.nativeNativeExNumberFormatExceptionTest();
            NativeExNumberFormatExceptionTest.processResult = NativeExNumberFormatExceptionTest.processResult - 10;
        } catch (NumberFormatException e1) {
            result1 = 3;
            NativeExNumberFormatExceptionTest.processResult = NativeExNumberFormatExceptionTest.processResult - 1;
        } catch (InvalidPathException e2) {
            result1 = 3;
            System.out.println("======>" + e2.getMessage());
            NativeExNumberFormatExceptionTest.processResult = NativeExNumberFormatExceptionTest.processResult - 10;
        } catch (StringIndexOutOfBoundsException e3) {
            result1 = 3;
            System.out.println("======>" + e3.getMessage());
            NativeExNumberFormatExceptionTest.processResult = NativeExNumberFormatExceptionTest.processResult - 10;
        }
        NativeExNumberFormatExceptionTest.processResult = NativeExNumberFormatExceptionTest.processResult - 3;
        return result1;
    }
}
