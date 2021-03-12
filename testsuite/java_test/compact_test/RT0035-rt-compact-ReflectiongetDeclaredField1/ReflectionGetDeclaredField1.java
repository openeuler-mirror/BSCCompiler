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
class GetDeclaredField1 {
    public int i = 1;
    String s = "aaa";
    private double d = 2.5;
    protected float f = -222;
}
public class ReflectionGetDeclaredField1 {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        try {
            Class zqp = Class.forName("GetDeclaredField1");
            Field zhu1 = zqp.getDeclaredField("i");
            Field zhu2 = zqp.getDeclaredField("s");
            Field zhu3 = zqp.getDeclaredField("d");
            Field zhu4 = zqp.getDeclaredField("f");
            if (zhu1.getName().equals("i") && zhu2.getName().equals("s") && zhu3.getName().equals("d") &&
                    zhu4.getName().equals("f")) {
                result = 0;
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            result = -1;
        } catch (NullPointerException e3) {
            System.err.println(e3);
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
