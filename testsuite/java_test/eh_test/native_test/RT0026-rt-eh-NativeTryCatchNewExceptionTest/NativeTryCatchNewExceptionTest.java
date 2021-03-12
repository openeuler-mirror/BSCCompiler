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
public class NativeTryCatchNewExceptionTest extends Exception {
    static {
        System.loadLibrary("jniNativeTryCatchNewExceptionTest");
    }
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.exit(run(argv, System.out));
    }
    private native void nativeNativeTryCatchNewExceptionTest() throws IllegalArgumentException;
    private void callback() throws NumberFormatException {
        throw new NumberFormatException("JavaMethodThrows");
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = nativeTryCatchNewExceptionTest();
        } catch (Exception e) {
            processResult--;
        }
        if (result == 2 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Asynchronous: Native(exception at callback from Java-throw new), captured by Java(try-catch).
     * @return status code
    */

    public static int nativeTryCatchNewExceptionTest() {
        int result1 = 4; /* STATUS_FAILED*/

        NativeTryCatchNewExceptionTest cTest = new NativeTryCatchNewExceptionTest();
        try {
            cTest.nativeNativeTryCatchNewExceptionTest();
            processResult -= 10;
        } catch (NumberFormatException e1) {
            processResult -= 10;
        } catch (IllegalStateException e2) {
            processResult -= 10;
        } finally {
            processResult -= 3;
        }
        return result1;
    }
}
