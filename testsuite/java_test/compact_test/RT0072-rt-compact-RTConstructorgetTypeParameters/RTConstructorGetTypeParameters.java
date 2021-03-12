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
import java.lang.reflect.TypeVariable;
class ConstructorGetTypeParameters {
    public <t, s, d, f, g> ConstructorGetTypeParameters(int number) {
    }
    ConstructorGetTypeParameters() {
    }
}
public class RTConstructorGetTypeParameters {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetTypeParameters");
            Constructor instance1 = cls.getConstructor(int.class);
            Constructor instance2 = cls.getDeclaredConstructor();
            TypeVariable[] q1 = instance1.getTypeParameters();
            TypeVariable[] q2 = instance2.getTypeParameters();
            if (q1.length == 5 && q2.length == 0) {
                System.out.println(0);
                return;
            }
            System.out.println(1);
            return;
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(3);
            return;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
