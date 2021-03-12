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
public class StringJoinNullPointerExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2;  /* STATUS_Success*/

        try {
            StringJoinNullPointerExceptionTest_1(null);
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 97) {
            result = 0;
        }
        return result;
    }
    private static void StringJoinNullPointerExceptionTest_1(String str) {
        try {
            String[] test = new String[]{str, str, str};
            System.out.println(String.join(null, test));
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
}
