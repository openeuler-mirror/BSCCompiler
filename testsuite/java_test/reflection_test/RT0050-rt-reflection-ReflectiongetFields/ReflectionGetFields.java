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
class GetFields_a {
    public int num = 5;
    String str = "bbb";
}
class GetFields extends GetFields_a {
    public int num = 1;
    String str = "aaa";
    private double dNum = 2.5;
    protected float fNum = -222;
}
interface GetFields_b {
    public int num = 2;
    String str = "ccc";
}
class GetFields_c {
}
public class ReflectionGetFields {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetFields");
            Class clazz2 = Class.forName("GetFields_b");
            Class clazz3 = Class.forName("GetFields_c");
            Field[] fields1 = clazz1.getFields();
            Field[] fields2 = clazz2.getFields();
            Field[] fields3 = clazz3.getFields();
            if (fields1.length == 2 && fields2.length == 2 && fields3.length == 0) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        }
    }
}