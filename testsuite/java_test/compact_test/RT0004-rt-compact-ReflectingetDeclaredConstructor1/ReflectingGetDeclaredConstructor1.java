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
class GetDeclaredConstructor1 {
    public GetDeclaredConstructor1() {
    }
    private GetDeclaredConstructor1(String name) {
    }
    protected GetDeclaredConstructor1(String name, int number) {
    }
    GetDeclaredConstructor1(int number) {
    }
}
public class ReflectingGetDeclaredConstructor1 {
    public static void main(String[] args) {
        try {
            Class getDeclaredConstructor1 = Class.forName("GetDeclaredConstructor1");
            Constructor constructor1 = getDeclaredConstructor1.getDeclaredConstructor(String.class);
            Constructor constructor2 = getDeclaredConstructor1.getDeclaredConstructor();
            Constructor constructor3 = getDeclaredConstructor1.getDeclaredConstructor(String.class, int.class);
            Constructor constructor4 = getDeclaredConstructor1.getDeclaredConstructor(int.class);
            if (constructor1.toString().equals("private GetDeclaredConstructor1(java.lang.String)")
                    && constructor2.toString().equals("public GetDeclaredConstructor1()")
                    && constructor3.toString().equals("protected GetDeclaredConstructor1(java.lang.String,int)")
                    && constructor4.toString().equals("GetDeclaredConstructor1(int)")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
