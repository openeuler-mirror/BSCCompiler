/*
 *- @TestCaseID: ClassInitFieldOtherMethod
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldOtherMethod.java
 *- @Title/Destination: When f is a static field of class One and call method except setXX/getXX, class is not
 *                      initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("One" , false, One.class.getClassLoader()) and clazz.getField to get a static field f of class.
 * -#step2: Call method of Field except setXX/getXX, class One is not initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldOtherMethod.java
 *- @ExecuteClass: ClassInitFieldOtherMethod
 *- @ExecuteArgs:
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
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
