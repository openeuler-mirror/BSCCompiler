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
public class StringBuilderExceptionsTest {
    private static int processResult = 99;
    public static void main(String argv[]) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = StringBuilderSubstringStringIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderSubstringStringIndexOutOfBoundsException_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderCodePointAtIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderCodePointBeforeIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderCodePointCountIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderReplaceStringIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderGetCharsIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBuilderOffsetByCodePointsIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 67) {
            result = 0;
        }
        return result;
    }
    // Test StringIndexOutOfBoundsException in String substring(int start, int end).
    public static int StringBuilderSubstringStringIndexOutOfBoundsException_1() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder str = new StringBuilder("java is boring");
        // Test end > str.length().
        try {
            String test1 = str.substring(8, 20);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test end < 0.
        try {
            String test1 = str.substring(8, -2);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test end < start.
        try {
            String test1 = str.substring(8, 6);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start > str.length().
        try {
            String test1 = str.substring(20, 6);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start < 0.
        try {
            String test1 = str.substring(-2, 6);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test StringIndexOutOfBoundsException in String substring(int start).
    public static int StringBuilderSubstringStringIndexOutOfBoundsException_2() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder str = new StringBuilder("java is boring");
        // Test start > str.length.
        try {
            String test1 = str.substring(20);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start < 0.
        try {
            String test1 = str.substring(-2);
            System.out.println(test1);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test IndexOutOfBoundsException in int codePointAt(int index).
    public static int StringBuilderCodePointAtIndexOutOfBoundsException_1() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder buff = new StringBuilder("programming");
        // Test index >= stringBuilder.length.
        try {
            int str = buff.codePointAt(11);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test index < 0.
        try {
            int str = buff.codePointAt(-1);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test IndexOutOfBoundsException in int codePointBefore(int index).
    public static int StringBuilderCodePointBeforeIndexOutOfBoundsException_1() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder buff = new StringBuilder("HelloWorld");
        // Test index > stringBuilder.length.
        try {
            int str = buff.codePointBefore(11);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test index < 0.
        try {
            int str = buff.codePointBefore(-3);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test IndexOutOfBoundsException in int codePointCount(int beginIndex, int endIndex).
    public static int StringBuilderCodePointCountIndexOutOfBoundsException_1() {
        int result1 = 4; /*STATUS_FAILED*/
        StringBuilder buff = new StringBuilder("HelloWorld");
        // Test endIndex > stringBuilder.length.
        try {
            int str = buff.codePointCount(5, 11);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test endIndex < beginIndex.
        try {
            int str = buff.codePointCount(5, 4);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test endIndex < 0.
        try {
            int str = buff.codePointCount(5, -2);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test beginIndex < 0.
        try {
            int str = buff.codePointCount(-1, 5);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test beginIndex > stringBuilder.length.
        try {
            int str = buff.codePointCount(11, 5);
            System.out.println(str);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test StringIndexOutOfBoundsException in StringBuilder replace(int start, int end, String str).
    public static int StringBuilderReplaceStringIndexOutOfBoundsException_1() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder str = new StringBuilder("Java Util Package");
        // Test end > stringBuilder.length.
        try {
            StringBuilder test1 = str.replace(18, 20, "Lang");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test end < start.
        try {
            StringBuilder test1 = str.replace(18, 16, "Lang");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test end < 0.
        try {
            StringBuilder test1 = str.replace(18, -2, "Lang");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start < 0.
        try {
            StringBuilder test1 = str.replace(-2, 16, "Lang");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start > stringBuilder.length.
        try {
            StringBuilder test1 = str.replace(20, 16, "Lang");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test IndexOutOfBoundsException in void getChars(int srcBegin, int srcEnd, char[] dst, int dstBegin).
    public static int StringBuilderGetCharsIndexOutOfBoundsException_1() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder str = new StringBuilder("java programming");
        char[] cArr = new char[]{'t', 'u', 't', 'o', 'r', 'i', 'a', 'l', 's'};
        // Test cArr.length - dstBegin < srcEnd - srcBegin
        try {
            str.getChars(5, 9, cArr, 6);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test dstBegin > cArr.length.
        try {
            str.getChars(0, 9, cArr, 10);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test dstBegin < 0.
        try {
            str.getChars(0, 9, cArr, -2);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcEnd < 0.
        try {
            str.getChars(0, -2, cArr, 0);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcBegin < 0.
        try {
            str.getChars(-2, 5, cArr, 0);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcBegin > srcEnd.
        try {
            str.getChars(5, 2, cArr, 0);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcBegin > stringBuilder.length.
        try {
            str.getChars(20, 2, cArr, 0);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcEnd > stringBuilder.length.
        try {
            str.getChars(2, 20, cArr, 0);
            System.out.println(str);
            System.out.println(cArr);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    // Test IndexOutOfBoundsException in int offsetByCodePoints(int index, int codePointOffset).
    public static int StringBuilderOffsetByCodePointsIndexOutOfBoundsException_1() {
        int result1 = 4; /* STATUS_FAILED*/

        StringBuilder str = new StringBuilder("abcdefg");
        // Test codePointOffset > stringBuilder.length - index.
        try {
            int result = str.offsetByCodePoints(1, 7);
            System.out.println(str);
            System.out.println(result);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test index < 0.
        try {
            int result = str.offsetByCodePoints(-1, 3);
            System.out.println(str);
            System.out.println(result);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test index > stringBuilder.length.
        try {
            int result = str.offsetByCodePoints(8, 0);
            System.out.println(str);
            System.out.println(result);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
}
