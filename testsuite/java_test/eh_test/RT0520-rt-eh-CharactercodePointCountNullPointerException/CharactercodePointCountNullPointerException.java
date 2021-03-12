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
public class CharactercodePointCountNullPointerException {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = charactercodePointCountNullPointerException();
        } catch (Exception e) {
            processResult -= 20;
        }
        try {
            result = charactercodePointCountNullPointerException2();
        } catch (Exception e) {
            processResult -= 40;
        }
        if (result == 4 && processResult == 97) {
            result = 0;
        }
        return result;
    }
    /**
     * codePointCount(char[] a, int offset, int count),a is null,catch NullPointerException
     * @return status code
    */

    public static int charactercodePointCountNullPointerException() {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - if seq is null.
        //
        // public static int codePointCount(char[] a, int offset, int count)
        int offset = 1;
        int count = 4;
        char[] chars = null;
        try {
            int obj = Character.codePointCount(chars, offset, count);
            processResult -= 10;
        } catch (NullPointerException e1) {
            processResult--;
        }
        return result1;
    }
    /**
     * codePointCount(CharSequence seq, int beginIndex, int endIndex), seq is null,catch NullPointerException
     * @return status code
    */

    public static int charactercodePointCountNullPointerException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - if seq is null.
        //
        // public static int codePointCount(CharSequence seq, int beginIndex, int endIndex)
        int offset = 1;
        int count = 4;
        CharSequence seq = null;
        try {
            int obj = Character.codePointCount(seq, offset, count);
            processResult -= 30;
        } catch (NullPointerException e1) {
            processResult--;
        }
        return result1;
    }
}