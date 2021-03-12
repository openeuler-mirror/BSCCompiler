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


interface A {
}
class A1 implements A {
}
class A2 extends A1 {
}
public class ReflectionAsSubclass1 {
    public static void main(String[] args) {
        try {
            Class a2 = Class.forName("A1").asSubclass(A.class);
            Class a1 = Class.forName("A2").asSubclass(A.class);
            if (a2.newInstance() instanceof A) {
                if (a1.newInstance() instanceof A) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(e1);
        } catch (InstantiationException e2) {
            System.out.println(e2);
        } catch (IllegalAccessException e3) {
            System.out.println(e3);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
