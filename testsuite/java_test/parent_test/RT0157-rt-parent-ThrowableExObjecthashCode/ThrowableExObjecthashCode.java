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


import java.lang.Throwable;
public class ThrowableExObjecthashCode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new ThrowableExObjecthashCode().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = throwableExObjecthashCode1();
        } catch (Exception e) {
            ThrowableExObjecthashCode.res = ThrowableExObjecthashCode.res - 20;
        }
        if (result == 4 && ThrowableExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private int throwableExObjecthashCode1() {
        int result1 = 4; /*STATUS_FAILED*/
        // int hashCode()
        Throwable cause1 = new Throwable("detailed message of cause1");
        Throwable cause2 = cause1;
        Throwable cause3 = new Throwable("detailed message of cause3");
        if (cause1.hashCode() == cause2.hashCode() && cause1.hashCode() != cause3.hashCode()) {
            ThrowableExObjecthashCode.res = ThrowableExObjecthashCode.res - 10;
        } else {
            ThrowableExObjecthashCode.res = ThrowableExObjecthashCode.res - 5;
        }
        return result1;
    }
}