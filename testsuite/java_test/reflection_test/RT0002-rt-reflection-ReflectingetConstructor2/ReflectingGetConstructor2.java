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
class GetConstructor2 {
    public GetConstructor2() {
    }
    public GetConstructor2(String name) {
    }
    public GetConstructor2(String name, int number) {
    }
    GetConstructor2(int number) {
    }
}
public class ReflectingGetConstructor2 {
    public static void main(String[] args) {
        try {
            Class getConstructor21 = Class.forName("GetConstructor2");
            Constructor constructor1 = getConstructor21.getConstructor(int.class);
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e) {
            try {
                Class getConstructor22 = Class.forName("GetConstructor2");
                Constructor constructor2 = getConstructor22.getConstructor(String.class, char.class, int.class);
                System.out.println(2);
            } catch (ClassNotFoundException ee) {
                System.err.println(ee);
                System.out.println(2);
            } catch (NoSuchMethodException ee) {
                System.out.println(0);
            }
        }
    }
}