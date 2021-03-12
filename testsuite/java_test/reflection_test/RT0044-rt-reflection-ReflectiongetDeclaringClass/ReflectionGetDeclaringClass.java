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


import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
class GetDeclaringClass {
    public int num;
    String str;
    float fNum;
    public GetDeclaringClass() {
    }
    GetDeclaringClass(int number) {
    }
    GetDeclaringClass(String name) {
    }
    GetDeclaringClass(int number, String name) {
    }
}
class GetDeclaringClass_a {
}
public class ReflectionGetDeclaringClass {
    public static void main(String[] args) {
        try {
            int num = 0;
            Class clazz = Class.forName("GetDeclaringClass");
            Field[] fields = clazz.getDeclaredFields();
            Constructor<?>[] constructors = clazz.getDeclaredConstructors();
            for (int i = 0; i < fields.length; i++) {
                if (fields[i].getDeclaringClass().getName().equals("GetDeclaringClass")) {
                    for (int j = 0; j < constructors.length; j++) {
                        if (constructors[j].getDeclaringClass().getName().equals("GetDeclaringClass")) {
                            Class clazz2 = Class.forName("GetDeclaringClass_a");
                            if (clazz2.getDeclaringClass() == null) {
                                num++;
                            }
                        }
                    }
                }
            }
            if (num == 12) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}