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


import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;
public class ParameterGetAnnotation {
    public static void main(String[] args) {
        int result = 2;
        try {
            result = ParameterGetAnnotation1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }
    public static int ParameterGetAnnotation1() {
        Method m;
        try {
            m = MyTargetTest18.class.getMethod("newMethod", new Class[] {String.class});
            Parameter[] parameters = m.getParameters();
            MyTarget Target = parameters[0].getAnnotation(MyTarget.class);
            if ("@ParameterGetAnnotation$MyTarget(name=name1, value=value1)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }
    class MyTargetTest18 {
        @MyTarget(name = "newName", value = "newValue")
        public String home;
        @MyTarget(name = "name", value = "value")
        public void MyTargetTest_1() {
            System.out.println("This is Example:hello world");
        }
        public void newMethod(@MyTarget(name = "name1", value = "value1") String home) {
            System.out.println("my home at:" + home);
        }
    }
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }
}