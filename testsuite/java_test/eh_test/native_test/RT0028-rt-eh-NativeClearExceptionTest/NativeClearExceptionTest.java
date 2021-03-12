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
public class NativeClearExceptionTest extends Exception {
    static {
        System.loadLibrary("jniNativeClearExceptionTest");
    }
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.exit(run(argv, System.out));
    }
    private native void nativeNativeClearExceptionTest() throws InvalidPathException;
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
            result = nativeClearExceptionTest();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Asynchronous: Native(exception at callback from Java, then Occurred-throw new-clear).
     * @return status code
    */

    public static int nativeClearExceptionTest() {
        int result1 = 4; /* STATUS_FAILED*/

        NativeClearExceptionTest cTest = new NativeClearExceptionTest();
        try {
            cTest.nativeNativeClearExceptionTest();
            processResult -= 1;
        } catch (NumberFormatException e1) {
            result1 = 3;
            System.out.println("======>" + e1.getMessage());
            processResult -= 10;
        } catch (InvalidPathException e2) {
            result1 = 3;
            System.out.println("======>" + e2.getMessage());
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e3) {
            result1 = 3;
            System.out.println("======>" + e3.getMessage());
            processResult -= 10;
        }
        processResult -= 3;
        return result1;
    }
}
