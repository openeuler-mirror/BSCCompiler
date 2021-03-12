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
public class StringSubSequenceIndexOutOfBoundsExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv,  PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = StringSubSequenceIndexOutOfBoundsExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    public static int StringSubSequenceIndexOutOfBoundsExceptionTest_1() {
        int result1 = 4; /*STATUS_FAILED*/
        //IndexOutOfBoundsException - if the index argument is negative or not less than the length of this string.
        String str1_1 = new String("abc123");
        try {
            str1_1.subSequence(-2, 6);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        try {
            str1_1.subSequence(2, -6);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        try {
            str1_1.subSequence(2, 1);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        try {
            str1_1.subSequence(2, 10);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
}