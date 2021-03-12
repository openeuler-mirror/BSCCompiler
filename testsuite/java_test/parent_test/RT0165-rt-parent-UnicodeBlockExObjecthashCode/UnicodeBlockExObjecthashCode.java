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
public class UnicodeBlockExObjecthashCode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new UnicodeBlockExObjecthashCode().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = unicodeBlockExObjecthashCode1();
        } catch (Exception e) {
            UnicodeBlockExObjecthashCode.res = UnicodeBlockExObjecthashCode.res - 20;
        }
        if (result == 4 && UnicodeBlockExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private int unicodeBlockExObjecthashCode1() {
        int result1 = 4; /*STATUS_FAILED*/
        // int hashCode()
        Character.UnicodeBlock unB1 = Character.UnicodeBlock.of(10);
        Character.UnicodeBlock unB2 = Character.UnicodeBlock.of(10);
        Character.UnicodeBlock unB3 = Character.UnicodeBlock.of('〆');
        if (unB1.hashCode() == unB2.hashCode() && unB1.hashCode() != unB3.hashCode()) {
            UnicodeBlockExObjecthashCode.res = UnicodeBlockExObjecthashCode.res - 10;
        } else {
            UnicodeBlockExObjecthashCode.res = UnicodeBlockExObjecthashCode.res - 5;
        }
        return result1;
    }
}