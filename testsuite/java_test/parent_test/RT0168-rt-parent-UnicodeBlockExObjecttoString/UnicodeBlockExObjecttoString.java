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


import java.lang.Character;
public class UnicodeBlockExObjecttoString {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new UnicodeBlockExObjecttoString().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = unicodeBlockExObjecttoString1();
        } catch (Exception e) {
            UnicodeBlockExObjecttoString.res = UnicodeBlockExObjecttoString.res - 20;
        }
        if (result == 4 && UnicodeBlockExObjecttoString.res == 89) {
            result = 0;
        }
        return result;
    }
    private int unicodeBlockExObjecttoString1() {
        int result1 = 4; /*STATUS_FAILED*/
        // final String toString()
        Character.UnicodeBlock unB1 = null;
        for (int cp = 0; cp <= 10; ++cp) {
            unB1 = Character.UnicodeBlock.of(cp);
        }
        String px1 = unB1.toString();
        if (px1.equals("BASIC_LATIN")) {
            UnicodeBlockExObjecttoString.res = UnicodeBlockExObjecttoString.res - 10;
        }
        return result1;
    }
}