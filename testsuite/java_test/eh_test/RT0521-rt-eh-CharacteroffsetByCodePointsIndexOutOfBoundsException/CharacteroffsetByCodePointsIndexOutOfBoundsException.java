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
public class CharacteroffsetByCodePointsIndexOutOfBoundsException {
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
            result = characteroffsetByCodePointsIndexOutOfBoundsException1();
        } catch (Exception e) {
            processResult -= 20;
        }
        try {
            result = characteroffsetByCodePointsIndexOutOfBoundsException2();
        } catch (Exception e) {
            processResult -= 40;
        }
        if (result == 4 && processResult == 97) {
            result = 0;
        }
        return result;
    }
    /**
     * offsetByCodePoints(char[] a, int start, int count, int index, int codePointOffset), check IndexOutOfBoundsException
     * @return status code
    */

    public static int characteroffsetByCodePointsIndexOutOfBoundsException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IndexOutOfBoundsException -if start or count is negative, or if start + count is larger than the length
        // of the given array, or if index is less than start or larger then start + count, or if codePointOffset
        // is positive and the text range starting with index and ending with start + count - 1 has fewer than
        // codePointOffset code points, or if codePointOffset is negative and the text range starting with start and
        // ending with index - 1 has fewer than the absolute value of codePointOffset code points.
        //
        // public static int offsetByCodePoints(char[] a, int start, int count, int index, int codePointOffset)
        int offset = 0;
        int count = 10;
        char[] chars = new char[] {'a', 'b', 'c', 'd', 'e'};
        try {
            int obj = Character.offsetByCodePoints(chars, offset, count, 0, 0);
            System.out.println(obj);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    /**
     * offsetByCodePoints(CharSequence seq, int index, int codePointOffset), catch IndexOutOfBoundsException
     * @return status code
    */

    public static int characteroffsetByCodePointsIndexOutOfBoundsException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IndexOutOfBoundsException -if start or count is negative, or if start + count is larger than the length
        // of the given array, or if index is less than start or larger then start + count, or if codePointOffset
        // is positive and the text range starting with index and ending with start + count - 1 has fewer than
        // codePointOffset code points, or if codePointOffset is negative and the text range starting with start and
        // ending with index - 1 has fewer than the absolute value of codePointOffset code points.
        //
        // public static int offsetByCodePoints(CharSequence seq, int index, int codePointOffset)
        int offset = 0;
        int count = 10;
        CharSequence seq = "a b c d e";
        try {
            int obj = Character.offsetByCodePoints(seq, offset, count);
            System.out.println(obj);
            processResult -= 30;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
}