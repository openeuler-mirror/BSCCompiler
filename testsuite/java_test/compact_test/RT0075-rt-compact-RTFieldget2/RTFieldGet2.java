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


class FieldGet2_a {
    public int num;
}
class FieldGet2 extends FieldGet2_a {
    private int number = 1;
    public boolean aBoolean = true;
    public int number1 = 8;
}
class FieldGet2_b {
    public int number1 = 18;
}
public class RTFieldGet2 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGet2");
            Object obj = cls.newInstance();
            Object q1 = cls.getDeclaredField("number").get(obj);
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
            try {
                Class cls = Class.forName("FieldGet2");
                Class cls1 = Class.forName("FieldGet2_b");
                Object instance1 = cls1.newInstance();
                Object q2 = cls.getDeclaredField("number1").get(instance1);
            } catch (ClassNotFoundException e5) {
                System.err.println(e5);
                System.out.println(2);
            } catch (InstantiationException e6) {
                System.err.println(e6);
                System.out.println(2);
            } catch (NoSuchFieldException e7) {
                System.err.println(e7);
                System.out.println(2);
            } catch (IllegalAccessException e8) {
                System.err.println(e8);
                System.out.println(2);
            } catch (IllegalArgumentException e9) {
                try {
                    Class cls = Class.forName("FieldGet2");
                    Object q3 = cls.getDeclaredField("aBoolean").get(null);
                } catch (ClassNotFoundException e10) {
                    System.err.println(e10);
                    System.out.println(2);
                } catch (NoSuchFieldException e11) {
                    System.err.println(e11);
                    System.out.println(2);
                } catch (IllegalAccessException e12) {
                    System.err.println(e12);
                    System.out.println(2);
                } catch (NullPointerException e13) {
                    System.out.println(0);
                }
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
