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


import java.lang.reflect.Field;
class GetDeclaredFields1_a {
    public int i_a = 5;
    String s_a = "bbb";
}
class GetDeclaredFields1 extends GetDeclaredFields1_a {
    public int i = 1;
    String s = "aaa";
    private double d = 2.5;
    protected float f = -222;
}
interface GetDeclaredFields1_b {
    public int i_b = 2;
    String s_b = "ccc";
}
public class ReflectionGetDeclaredFields1 {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        try {
            Class zqp1 = Class.forName("GetDeclaredFields1");
            Class zqp2 = Class.forName("GetDeclaredFields1_b");
            Field[] j = zqp1.getDeclaredFields();
            Field[] k = zqp2.getDeclaredFields();
            if (j.length == 4 && k.length == 2) {
                result = 0;
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
