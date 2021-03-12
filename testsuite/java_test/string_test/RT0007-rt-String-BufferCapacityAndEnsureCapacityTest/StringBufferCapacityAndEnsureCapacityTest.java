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
public class StringBufferCapacityAndEnsureCapacityTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBufferCapacityAndEnsureCapacityTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBufferCapacityAndEnsureCapacityTest_1() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=" +
                "!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test1(strBuffer1_1);
        test1(strBuffer1_2);
        test1(strBuffer1_3);
        test1(strBuffer1_4);
        test1(strBuffer1_5);
    }
    private static void test1(StringBuffer strBuffer) {
        System.out.println("1-capacity: " + strBuffer.capacity());
        System.out.println("1-lent: " + strBuffer.length());
        strBuffer = strBuffer.append("01234567890123456789");
        System.out.println("2-capacity: " + strBuffer.capacity());
        System.out.println("2-lent: " + strBuffer.length());
        strBuffer.ensureCapacity(80);
        System.out.println("3-capacity: " + strBuffer.capacity());
        System.out.println("3-lent: " + strBuffer.length());
        strBuffer.setLength(100);
        System.out.println("4-capacity: " + strBuffer.capacity());
        System.out.println("4-lent: " + strBuffer.length());
        strBuffer.trimToSize();
        System.out.println("5-capacity: " + strBuffer.capacity());
        System.out.println("5-lent: " + strBuffer.length());
        // Test minimumCapacity < 0.
        strBuffer.ensureCapacity(-2);
        System.out.println("6-capacity: " + strBuffer.capacity());
        System.out.println("6-lent: " + strBuffer.length());
        // Test minimumCapacity = 0.
        strBuffer.ensureCapacity(0);
        System.out.println("7-capacity: " + strBuffer.capacity());
        System.out.println("7-lent: " + strBuffer.length());
        // Test minimumCapacity > 0.
        strBuffer.ensureCapacity(2);
        System.out.println("8-capacity: " + strBuffer.capacity());
        System.out.println("8-lent: " + strBuffer.length());
    }
}
