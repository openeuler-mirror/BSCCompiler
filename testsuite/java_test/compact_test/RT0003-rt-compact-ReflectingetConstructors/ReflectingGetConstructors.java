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
class GetConstructors {
    public GetConstructors() {
    }
    public GetConstructors(String name) {
    }
    public GetConstructors(String name, int number) {
    }
    GetConstructors(int number) {
    }
    GetConstructors(double id) {
    }
    public GetConstructors(double id, String name) {
    }
}
class GetConstructors_a extends GetConstructors {
    public GetConstructors_a() {
    }
    public GetConstructors_a(String name) {
    }
    public GetConstructors_a(String name, int number) {
    }
    GetConstructors_a(int number) {
    }
}
public class ReflectingGetConstructors {
    public static void main(String[] args) {
        try {
            Class getConstructors_a = Class.forName("GetConstructors_a");
            Constructor<?>[] constructors = getConstructors_a.getConstructors();
            if (constructors.length == 3) {
                for (int i = 0; i < constructors.length; i++) {
                    if (constructors[i].toString().indexOf("GetConstructors_a(int)") != -1) {
                        System.out.println(2);
                    }
                }
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (ArrayIndexOutOfBoundsException e1) {
            System.err.println(e1);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
