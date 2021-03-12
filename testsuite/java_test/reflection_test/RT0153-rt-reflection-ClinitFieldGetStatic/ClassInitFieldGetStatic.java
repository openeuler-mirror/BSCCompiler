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


import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Field;
public class ClassInitFieldGetStatic {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        String fValue = "";
        try {
            Class clazz = Class.forName("One", false, One.class.getClassLoader());
            Field f = clazz.getField("hi");
            if (result.toString().compareTo("") == 0) {
                fValue = (String)f.get(null);
            }
        } catch (Exception e) {
            System.out.println(e);
        }
        if (result.toString().compareTo("SuperOne") == 0 && fValue.compareTo("hi") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}
@A
class Super {
    static {
        ClassInitFieldGetStatic.result.append("Super");
    }
}
interface InterfaceSuper {
    String a = ClassInitFieldGetStatic.result.append("|InterfaceSuper|").toString();
}
@A(i=1)
class One extends Super implements InterfaceSuper {
    static {
        ClassInitFieldGetStatic.result.append("One");
    }
    public static String hi = "hi";
}
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@interface A {
    int i() default 0;
    String a = ClassInitFieldGetStatic.result.append("|InterfaceA|").toString();
}
class Two extends One {
    static {
        ClassInitFieldGetStatic.result.append("Two");
    }
}
