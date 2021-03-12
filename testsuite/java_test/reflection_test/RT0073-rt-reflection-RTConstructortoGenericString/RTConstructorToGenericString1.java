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
class ConstructorToGenericString1 {
    public <t, s, d, f, g> ConstructorToGenericString1(int number) {
    }
}
public class RTConstructorToGenericString1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorToGenericString1");
            Constructor instance1 = cls.getConstructor(int.class);
            if (instance1.toGenericString().equals("public <t,s,d,f,g> ConstructorToGenericString1(int)")) {
                System.out.println(0);
                return;
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(1);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(2);
            return;
        }
        System.out.println(3);
        return;
    }
}
