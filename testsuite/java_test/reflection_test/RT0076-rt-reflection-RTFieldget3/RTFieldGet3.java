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


class FieldGet3_a {
    public int num;
}
class FieldGet3 extends FieldGet3_a {
    public static String str;
}
public class RTFieldGet3 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGet3");
            Object obj = cls.newInstance();
            Object obj1 = cls.getField("str").get(obj);
            Object obj2 = cls.getField("num").get(obj);
            if (obj1 == null && (int) obj2 == 0) {
                System.out.println(0);
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