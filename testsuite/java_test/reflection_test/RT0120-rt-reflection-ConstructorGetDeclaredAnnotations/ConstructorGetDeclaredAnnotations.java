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
import java.lang.reflect.Constructor;
public class ConstructorGetDeclaredAnnotations {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }
    public static void main(String[] args) {
        int result = 2;
        try {
            result = ConstructorGetDeclaredAnnotations1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }
    public static int ConstructorGetDeclaredAnnotations1() {
        Constructor<MyTargetTest01> m;
        try {
            m = MyTargetTest01.class.getConstructor(new Class[] {ConstructorGetDeclaredAnnotations.class, String.class});
            MyTarget Target = m.getDeclaredAnnotation(MyTarget.class);
            if ("@ConstructorGetDeclaredAnnotations$MyTarget(name=cons, value=constructor)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }
    class MyTargetTest01 {
        @MyTarget(name = "newName", value = "newValue")
        public String home;
        @MyTarget(name = "name", value = "value")
        public void MyTargetTest_1() {
            System.out.println("This is Example:hello world");
        }
        public void newMethod(@MyTarget(name = "name1", value = "value1") String home) {
            System.out.println("my home at:" + home);
        }
        @MyTarget(name = "cons", value = "constructor")
        public MyTargetTest01(String home) {
            this.home = home;
        }
    }
}