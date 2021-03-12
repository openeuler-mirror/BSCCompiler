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


import java.lang.reflect.Field;
public class FieldExObjectgetClass {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new FieldExObjectgetClass().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExObjectgetClass1();
        } catch (Exception e) {
            FieldExObjectgetClass.res = FieldExObjectgetClass.res - 20;
        }
        if (result == 4 && FieldExObjectgetClass.res == 89) {
            result = 0;
        }
        return result;
    }
    private int fieldExObjectgetClass1() {
        //  final Class<?> getClass()
        int result1 = 4; /*STATUS_FAILED*/
        Field[] f1 = FieldExObjectgetClass.class.getDeclaredFields();
        Class px1 = f1[0].getClass();
        if (px1.toString().equals("class java.lang.reflect.Field")) {
            FieldExObjectgetClass.res = FieldExObjectgetClass.res - 10;
        }
        return result1;
    }
}