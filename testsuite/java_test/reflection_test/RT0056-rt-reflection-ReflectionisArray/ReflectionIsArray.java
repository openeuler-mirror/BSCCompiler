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
class IsArray {
    public int num;
    String str;
    float fNum;
}
public class ReflectionIsArray {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("IsArray");
            Field[] fields = clazz.getDeclaredFields();
            Class clazz1 = fields.getClass();
            if (clazz1.isArray()) {
                if (!clazz.isArray()) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}