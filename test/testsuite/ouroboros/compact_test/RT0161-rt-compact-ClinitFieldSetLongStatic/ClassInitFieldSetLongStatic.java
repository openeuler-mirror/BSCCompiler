/*
 *- @TestCaseID: ClassInitFieldSetLongStatic
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldSetLongStatic.java
 *- @Title/Destination: When f is a static field of class One and call f.setLong(), class One is initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("One" , false, One.class.getClassLoader()) and clazz.getField to get a static field f of class
 *          One.
 * -#step2: Call method f.setLong(null, newValue), class One is initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldSetLongStatic.java
 *- @ExecuteClass: ClassInitFieldSetLongStatic
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Field;

public class ClassInitFieldSetLongStatic {
    static StringBuffer result = new StringBuffer("");

    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("One", false, One.class.getClassLoader());
            Field f = clazz.getField("hiLong");
            if (result.toString().compareTo("") == 0) {
                f.setLong(null, 25l);
            }
        } catch (Exception e) {
            System.out.println(e);
        }
        if (result.toString().compareTo("SuperOne") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}

@A
class Super {
    static {
        ClassInitFieldSetLongStatic.result.append("Super");
    }
}

interface InterfaceSuper {
    String a = ClassInitFieldSetLongStatic.result.append("|InterfaceSuper|").toString();
}

@A(i=1)
class One extends Super implements InterfaceSuper {
    static {
        ClassInitFieldSetLongStatic.result.append("One");
    }

    public static long hiLong = 4859l;
}

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@interface A {
    int i() default 0;
    String a = ClassInitFieldSetLongStatic.result.append("|InterfaceA|").toString();
}

class Two extends One {
    static {
        ClassInitFieldSetLongStatic.result.append("Two");
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
