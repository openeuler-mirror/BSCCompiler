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
class GetDeclaredConstructor2 {
    public GetDeclaredConstructor2() {
    }
    private GetDeclaredConstructor2(String name) {
    }
    protected GetDeclaredConstructor2(String name, int number) {
    }
    GetDeclaredConstructor2(int number) {
    }
}
public class ReflectingGetDeclaredConstructor2 {
    public static void main(String[] args) {
        try {
            Class getDeclaredConstructor2 = Class.forName("GetDeclaredConstructor2");
            Constructor constructor = getDeclaredConstructor2.getDeclaredConstructor(String.class, char.class, int.class);
            System.out.println(2);
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.out.println(0);
        }
    }
}