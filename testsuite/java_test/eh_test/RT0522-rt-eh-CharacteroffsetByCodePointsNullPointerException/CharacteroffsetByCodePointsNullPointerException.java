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
public class CharacteroffsetByCodePointsNullPointerException {
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
        ;
        try {
            result = characteroffsetByCodePointsNullPointerException1();
        } catch (Exception e) {
            processResult -= 20;
        }
        try {
            result = characteroffsetByCodePointsNullPointerException2();
        } catch (Exception e) {
            processResult -= 40;
        }
        if (result == 4 && processResult == 97) {
            result = 0;
        }
        return result;
    }
    /**
     * offsetByCodePoints(char[] a, int start, int count, int index, int codePointOffset), catch NullPointerException
     * @return status code
    */

    public static int characteroffsetByCodePointsNullPointerException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - if seq is null.
        //
        // public static int offsetByCodePoints(char[] a, int start, int count, int index, int codePointOffset)
        int offset = 1;
        int count = 4;
        char[] chars = null;
        try {
            int obj = Character.offsetByCodePoints(chars, offset, count, 0, 2);
            processResult -= 10;
        } catch (NullPointerException e1) {
            processResult--;
        }
        return result1;
    }
    /**
     * offsetByCodePoints(CharSequence seq, int index, int codePointOffset), catch NullPointerException
     * @return status code
    */

    public static int characteroffsetByCodePointsNullPointerException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - if seq is null.
        //
        // public static int offsetByCodePoints(CharSequence seq, int index, int codePointOffset)
        int offset = 1;
        int count = 4;
        CharSequence seq = null;
        try {
            int obj = Character.offsetByCodePoints(seq, offset, count);
            processResult -= 30;
        } catch (NullPointerException e1) {
            processResult--;
        }
        return result1;
    }
}
