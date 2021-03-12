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


import java.lang.Class;
public class StringExObjectgetClass {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new StringExObjectgetClass().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = stringExObjectgetClass1();
        } catch (Exception e) {
            StringExObjectgetClass.res = StringExObjectgetClass.res - 20;
        }
        if (result == 4 && StringExObjectgetClass.res == 89) {
            result = 0;
        }
        return result;
    }
    private int stringExObjectgetClass1() {
        //  final Class<?> getClass()
        int result1 = 4; /*STATUS_FAILED*/
        String tr2 = "this is a test";
        Class<? extends String> cls1 = tr2.getClass();
        if (cls1.toString().equals("class java.lang.String")) {
            StringExObjectgetClass.res = StringExObjectgetClass.res - 10;
        }
        return result1;
    }
}