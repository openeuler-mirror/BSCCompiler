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
class GetField2_a {
    int num = 5;
    public String str = "bbb";
}
class GetField2 extends GetField2_a {
    public int num = 1;
    String str2 = "aaa";
    private double dNum = 2.5;
    protected float fNum = -222;
}
public class ReflectionGetField2 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetField2");
            Field field1 = clazz1.getField("str2");
            System.out.println(2);
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            try {
                Class clazz2 = Class.forName("GetField2");
                Field field2 = clazz2.getField(null);
                System.out.println(2);
            } catch (ClassNotFoundException e4) {
                System.err.println(e4);
                System.out.println(2);
            } catch (NoSuchFieldException e5) {
                System.err.println(e5);
                System.out.println(2);
            } catch (NullPointerException e6) {
                System.out.println(0);
            }
        } catch (NullPointerException e3) {
            System.err.println(e3);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
