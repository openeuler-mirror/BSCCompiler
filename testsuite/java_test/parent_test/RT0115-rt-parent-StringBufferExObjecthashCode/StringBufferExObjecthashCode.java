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


import java.lang.StringBuffer;
public class StringBufferExObjecthashCode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new StringBufferExObjecthashCode().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = stringBufferExObjecthashCode1();
        } catch (Exception e) {
            StringBufferExObjecthashCode.res = StringBufferExObjecthashCode.res - 20;
        }
        if (result == 4 && StringBufferExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private int stringBufferExObjecthashCode1() {
        int result1 = 4; /*STATUS_FAILED*/
        // int hashCode()
        StringBuffer sb1 = new StringBuffer("aa");
        StringBuffer sb2 = sb1;
        StringBuffer sb3 = new StringBuffer("╬の〆");
        if (sb1.hashCode() == sb2.hashCode() && sb1.hashCode() != sb3.hashCode()) {
            StringBufferExObjecthashCode.res = StringBufferExObjecthashCode.res - 10;
        } else {
            StringBufferExObjecthashCode.res = StringBufferExObjecthashCode.res - 5;
        }
        return result1;
    }
}