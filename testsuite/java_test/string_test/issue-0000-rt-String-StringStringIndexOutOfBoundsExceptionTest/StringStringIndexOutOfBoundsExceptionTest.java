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
import java.io.UnsupportedEncodingException;
public class StringStringIndexOutOfBoundsExceptionTest {
    private static int processResult = 99;
    public static void main(String argv[]) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = StringStringIndexOutOfBoundsExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 96) {
            result = 0;
        }
//        System.out.println("result: " + result);
//        System.out.println("processResult:" + processResult);
        return result;
    }
    public static int StringStringIndexOutOfBoundsExceptionTest_1() throws UnsupportedEncodingException {
        int result = 4; /*STATUS_FAILED*/
        byte[] str1_1 = new byte[]{(byte) 0x61, (byte) 0x62, (byte) 0x63, (byte) 0x31, (byte) 0x32, (byte) 0x33};
        try {
            String str1 = new String(str1_1, 0, 3, "ASC");
            processResult -= 10;
        } catch (UnsupportedEncodingException e1) {
            processResult--;
        }
        try {
            String str2 = new String(str1_1, -1, 3, "ASCII");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult -= 1;
        }
        try {
            String str3 = new String(str1_1, 0, 8, "ASCII");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        return result;
    }
}
