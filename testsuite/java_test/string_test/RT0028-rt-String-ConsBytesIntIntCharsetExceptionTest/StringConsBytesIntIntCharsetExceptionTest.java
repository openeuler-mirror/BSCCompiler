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
import java.nio.charset.Charset;
public class StringConsBytesIntIntCharsetExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv,  PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            result = StringConsBytesIntIntCharsetExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 96) {
            result = 0;
        }
        return result;
    }
    public static int StringConsBytesIntIntCharsetExceptionTest_1() {
        int result1 = 2; /* STATUS_Success*/

        // IndexOutOfBoundsException - If the offset and length arguments index characters outside the bounds of the
        // bytes array
        byte[] str1_1 = new byte[]{(byte) 0x61, (byte) 0x62, (byte) 0x63, (byte) 0x31, (byte) 0x32, (byte) 0x33};
        try {
            String str1 = new String(str1_1, -1, 3, Charset.forName("ASCII"));
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        try {
            String str1 = new String(str1_1, 3, -1, Charset.forName("ASCII"));
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        try {
            String str1 = new String(str1_1, 3, 10, Charset.forName("ASCII"));
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
}
