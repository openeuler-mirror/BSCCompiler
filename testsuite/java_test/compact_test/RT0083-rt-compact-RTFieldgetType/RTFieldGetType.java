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
class FieldGetType {
    public static short sNum;
    final char aChar = '什';
    private float fNum;
}
public class RTFieldGetType {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetType");
            Field instance1 = cls.getField("sNum");
            Field instance2 = cls.getDeclaredField("aChar");
            Field instance3 = cls.getDeclaredField("fNum");
            if (instance1.getType().getName().equals("short") && instance2.getType().getName().equals("char")
                    && instance3.getType().getName().equals("float")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
