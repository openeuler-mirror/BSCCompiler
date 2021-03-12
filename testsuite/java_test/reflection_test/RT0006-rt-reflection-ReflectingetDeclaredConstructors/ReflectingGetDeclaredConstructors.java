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
class GetDeclaredConstructors {
    public GetDeclaredConstructors() {
    }
    private GetDeclaredConstructors(String name) {
    }
    protected GetDeclaredConstructors(String name, int number) {
    }
    GetDeclaredConstructors(int number) {
    }
}
public class ReflectingGetDeclaredConstructors {
    public static void main(String[] args) {
        try {
            Class getDeclaredConstructors = Class.forName("GetDeclaredConstructors");
            Constructor<?>[] constructors = getDeclaredConstructors.getDeclaredConstructors();
            if (constructors.length == 4) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
        }
    }
}