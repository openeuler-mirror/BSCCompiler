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
class GetConstructor1 {
    public GetConstructor1() {
    }
    public GetConstructor1(String name) {
    }
    public GetConstructor1(String name, int number) {
    }
    GetConstructor1(int number) {
    }
}
public class ReflectingGetConstructor1 {
    public static void main(String[] args) {
        try {
            Class getConstructor1 = Class.forName("GetConstructor1");
            Constructor constructor1 = getConstructor1.getConstructor(String.class);
            Constructor constructor2 = getConstructor1.getConstructor();
            Constructor constructor3 = getConstructor1.getConstructor(String.class, int.class);
            if (constructor1.toString().equals("public GetConstructor1(java.lang.String)")
                    && constructor2.toString().equals("public GetConstructor1()")
                    && constructor3.toString().equals("public GetConstructor1(java.lang.String,int)")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.out.println(2);
        }
    }
}