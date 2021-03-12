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
import java.lang.reflect.TypeVariable;
public class ClassGetTypeParameters {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }
    public static void main(String[] args) {
        int result = 2;
        try {
            result = ClassGetTypeParameters1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }
    public static int ClassGetTypeParameters1() {
        Class<MyTargetTest5> m;
        try {
            m = MyTargetTest5.class;
            TypeVariable<Class<MyTargetTest5>>[] target = m.getTypeParameters();
            if (target.length == 0) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }
    class MyTargetTest5 {
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
        public MyTargetTest5(String home) {
            this.home = home;
        }
    }
}