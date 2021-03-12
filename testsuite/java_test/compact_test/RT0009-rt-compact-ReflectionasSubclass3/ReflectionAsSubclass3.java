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


interface C {
}
class C_1 implements C {
}
class C_2 extends C_1 {
}
class D {
}
class D_1 extends D {
}
public class ReflectionAsSubclass3 {
    public static void main(String[] args) {
        try {
            Class.forName("C_1").asSubclass(D.class);
        } catch (ClassCastException e1) {
            try {
                Class.forName("C_2").asSubclass(D.class);
            } catch (ClassCastException e2) {
                try {
                    Class.forName("D_1").asSubclass(C.class);
                } catch (ClassCastException e3) {
                    System.out.println(0);
                } catch (ClassNotFoundException e4) {
                    System.out.println(2);
                }
            } catch (ClassNotFoundException e5) {
                System.out.println(2);
            }
        } catch (ClassNotFoundException e6) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
