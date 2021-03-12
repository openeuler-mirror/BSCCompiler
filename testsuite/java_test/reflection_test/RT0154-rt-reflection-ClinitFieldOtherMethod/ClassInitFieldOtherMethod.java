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


import java.lang.annotation.*;
import java.lang.reflect.Field;
public class ClassInitFieldOtherMethod {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("One", false, One.class.getClassLoader());
            Field f = clazz.getField("hi");
            f.equals(f);
            f.getAnnotation(B.class);
            f.getAnnotationsByType(B.class);
            f.getDeclaredAnnotations();
            f.getDeclaringClass();
            f.getGenericType();
            f.getModifiers();
            f.getName();
            f.getType();
            f.hashCode();
            f.isEnumConstant();
            f.isSynthetic();
            f.toGenericString();
            f.toString();
        } catch (Exception e) {
            System.out.println(e);
        }
        if(result.toString().compareTo("") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}
@A
class Super {
    static {
        ClassInitFieldOtherMethod.result.append("Super");
    }
}
interface InterfaceSuper {
    String a = ClassInitFieldOtherMethod.result.append("|InterfaceSuper|").toString();
}
@A(i=1)
class One extends Super implements InterfaceSuper {
    static {
        ClassInitFieldOtherMethod.result.append("One");
    }
    @B("hello")
    public static String hi = "value";
}
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@interface A {
    int i() default 0;
    String a = ClassInitFieldOtherMethod.result.append("|InterfaceA|").toString();
}
@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@interface B{
    String value() default "hi";
}
class Two extends One {
    static {
        ClassInitFieldOtherMethod.result.append("Two");
    }
}