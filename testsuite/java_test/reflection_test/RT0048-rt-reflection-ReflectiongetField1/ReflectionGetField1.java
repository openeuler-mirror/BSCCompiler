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
class GetField1_a {
    int num2 = 5;
    public String str = "bbb";
}
class GetField1 extends GetField1_a {
    public int num = 1;
    String string = "aaa";
    private double dNum = 2.5;
    protected float fNum = -222;
}
public class ReflectionGetField1 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetField1");
            Field field1 = clazz.getField("num");
            Field field2 = clazz.getField("str");
            if (field1.getName().equals("num") && field2.getName().equals("str")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        } catch (NullPointerException e3) {
            System.err.println(e3);
            System.out.println(2);
        }
    }
}