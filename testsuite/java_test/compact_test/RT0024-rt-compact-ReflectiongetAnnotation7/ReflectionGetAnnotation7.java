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


import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
class GetAnnotation7 {
    @Deprecated
    public int aa(int num) {
        return 0;
    }
}
class GetAnnotation7_a extends GetAnnotation7 {
    @Override
    public int aa(int num) {
        return 2;
    }
}
public class ReflectionGetAnnotation7 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation7_a");
            Method zhu1 = zqp1.getMethod("aa", int.class);
            Annotation[] j = zhu1.getAnnotations();
            if (j.length == 0) {
                try {
                    Class zqp2 = Class.forName("GetAnnotation7");
                    Method zhu2 = zqp2.getMethod("aa", int.class);
                    if (zhu2.getAnnotation(Deprecated.class).toString().indexOf("Deprecated") != -1) {
                        System.out.println(0);
                    }
                } catch (ClassNotFoundException e1) {
                    System.err.println(e1);
                    System.out.println(2);
                } catch (NoSuchMethodException e2) {
                    System.err.println(e2);
                    System.out.println(2);
                } catch (NullPointerException e3) {
                    System.err.println(e3);
                    System.out.println(2);
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        } catch (NullPointerException e3) {
            System.err.println(e3);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
