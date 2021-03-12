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
public class StringConsIntsIntIntTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringConsIntsIntIntTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringConsIntsIntIntTest_1() {
        int[] charIntArray1_1 = new int[]{97, 98, 99, 49, 50, 51};
        String str1_1 = new String(charIntArray1_1, 0, charIntArray1_1.length);
        String str1_2 = new String(charIntArray1_1, charIntArray1_1.length - 1, 1);
        String str1_3 = new String(charIntArray1_1, charIntArray1_1.length - 1, 0);
        System.out.println(str1_1);
        System.out.println(str1_2);
        System.out.println(str1_3);
        int[] charIntArray2_1 = new int[]{0x61, 0x62, 0x63, 0x31, 0x32, 0x33};
        String str2_1 = new String(charIntArray2_1, 0, charIntArray2_1.length);
        String str2_2 = new String(charIntArray2_1, charIntArray2_1.length - 1, 1);
        String str2_3 = new String(charIntArray2_1, charIntArray2_1.length - 1, 0);
        System.out.println(str2_1);
        System.out.println(str2_2);
        System.out.println(str2_3);
    }
}
