/*
 *- @TestCaseID: ClassInitFieldGetStatic
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldGetStatic.java
 *- @Title/Destination: When f is a static field of class One and call f.get(), class One is initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("One" , false, One.class.getClassLoader()) and clazz.getField to get a static field f of class.
 * -#step2: Call method f.get(null), class One is initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldGetStatic.java
 *- @ExecuteClass: ClassInitFieldGetStatic
 *- @ExecuteArgs:
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
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
