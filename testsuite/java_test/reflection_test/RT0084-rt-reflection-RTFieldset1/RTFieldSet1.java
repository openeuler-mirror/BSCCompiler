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
class FieldSet1_a {
    public static String str = "aaa";
    public int num2 = 2;
    public int number = 5;
    public static int num1;
}
class FieldSet1 extends FieldSet1_a {
    public static String str = "bbb";
    public int num = 1;
    public int test = super.number + 1;
    public static boolean aBoolean = true;
    public char aChar;
}
public class RTFieldSet1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldSet1");
            Object obj = cls.newInstance();
            Field f1 = cls.getField("str");
            Field f2 = cls.getField("num");
            Field f3 = cls.getField("aBoolean");
            Field f4 = cls.getField("aChar");
            Field f5 = cls.getField("num1");
            Field f6 = cls.getField("num2");
            Field f7 = cls.getField("number");
            f1.set(obj, "ccc");
            f2.set(obj, 10);
            f3.set(obj, false);
            f4.set(obj, '国');
            f5.set(obj, 20);
            f6.set(obj, 30);
            f7.set(obj, 40);
            if (f1.get(obj).toString().equals("ccc") && (int) f2.get(obj) == 10 && !(boolean) f3.get(obj) && (int)
                    f6.get(obj) == 30) {
                if (f4.get(obj).toString().equals("国") && (int) f5.get(obj) == 20) {
                    if ((int) cls.getField("test").get(obj) == 6) {
                        Class cls1 = Class.forName("FieldSet1_a");
                        Object instance1 = cls1.newInstance();
                        Object p = cls1.getDeclaredField("str").get(instance1);
                        if (p.toString().equals("aaa")) {
                            System.out.println(0);
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (InstantiationException e2) {
            System.err.println(e2);
            System.out.println(2);
        } catch (NoSuchFieldException e3) {
            System.err.println(e3);
            System.out.println(2);
        } catch (IllegalAccessException e4) {
            System.err.println(e4);
            System.out.println(2);
        }
    }
}