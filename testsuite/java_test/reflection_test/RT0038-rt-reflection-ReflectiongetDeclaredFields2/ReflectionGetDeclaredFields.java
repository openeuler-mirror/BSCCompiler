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
class GetDeclaredFields_a {
    public int i_a = 5;
    String s_a = "bbb";
}
class GetDeclaredFields extends GetDeclaredFields_a {
}
enum GetDeclaredFields_b {
    i_b, s_b, f_b
}
public class ReflectionGetDeclaredFields {
    public static void main(String[] args) {
        int result = 0;
        try {
            Class zqp1 = Class.forName("GetDeclaredFields");
            Class zqp2 = Class.forName("GetDeclaredFields_b");
            Field[] j = zqp1.getDeclaredFields();
            Field[] k = zqp2.getDeclaredFields();
            if (j.length == 0 && k.length == 4) {
                result = 0;
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        }
        System.out.println(result);
    }
}