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


import java.lang.reflect.Array;
public class ArrayExObjecthashCode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new ArrayExObjecthashCode().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = arrayExObjecthashCode1();
        } catch (Exception e) {
            ArrayExObjecthashCode.res = ArrayExObjecthashCode.res - 20;
        }
        if (result == 4 && ArrayExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private int arrayExObjecthashCode1() throws NoSuchFieldException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // int hashCode()
        Object ary0 = Array.newInstance(int.class, 10);
        Object ary1 = ary0;
        Object ary2 = Array.newInstance(int.class, 11);
        int px0 = ary0.hashCode();
        int px1 = ary1.hashCode();
        int px2 = ary2.hashCode();
        if (px0 == px1 && px0 != px2) {
            ArrayExObjecthashCode.res = ArrayExObjecthashCode.res - 10;
        }
        return result1;
    }
}