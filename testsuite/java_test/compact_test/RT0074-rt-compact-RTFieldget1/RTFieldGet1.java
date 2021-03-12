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


class FieldGet1_a {
    public static String str = "aaa";
    public int num = 2;
    public int num1 = 5;
    public static boolean aBoolean = false;
}
class FieldGet1 extends FieldGet1_a {
    public static String str = "bbb";
    public int number = 1;
    public int test = super.num1 + 1;
    public static boolean bBoolean = true;
    public char aChar = '国';
}
public class RTFieldGet1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGet1");
            Object obj = cls.newInstance();
            Object q1 = cls.getField("str").get(obj);
            Object q2 = cls.getField("number").get(obj);
            Object q3 = cls.getField("test").get(obj);
            Object q4 = cls.getField("bBoolean").get(obj);
            Object q5 = cls.getField("aChar").get(obj);
            Object q6 = cls.getField("num").get(obj);
            Object q7 = cls.getField("aBoolean").get(null);
            if (q1.toString().equals("bbb") && (int) q2 == 1 && (int) q3 == 6 && (boolean) q4 && q5.toString().
                    equals("国") && (int) q6 == 2) {
                Class cls1 = Class.forName("FieldGet1_a");
                Object instance1 = cls1.newInstance();
                Object q8 = cls1.getField("str").get(instance1);
                if (q8.toString().equals("aaa")) {
                    if (!(boolean) q7) {
                        System.out.println(0);
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
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
