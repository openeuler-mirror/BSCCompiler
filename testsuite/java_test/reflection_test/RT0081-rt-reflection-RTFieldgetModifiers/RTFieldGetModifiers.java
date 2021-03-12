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
class FieldGetModifiers {
    public static int num;
    final String str = "aaa";
    private String string = "ccc";
    protected static int number;
}
public class RTFieldGetModifiers {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetModifiers");
            Field instance1 = cls.getField("num");
            Field instance2 = cls.getDeclaredField("str");
            Field instance3 = cls.getDeclaredField("string");
            Field field = cls.getDeclaredField("number");
            if (instance1.getModifiers() == 9 && instance2.getModifiers() == 16 && instance3.getModifiers() == 2
                    && field.getModifiers()
                    == 12) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}