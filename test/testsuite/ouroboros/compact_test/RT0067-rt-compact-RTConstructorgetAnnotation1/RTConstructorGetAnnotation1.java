/*
 *- @TestCaseID: RTConstructorGetAnnotation1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTConstructorGetAnnotation1.java
 *- @Title/Destination: Use Constructor.getAnnotation() to get the annotation of the target class's constructor by 
 *                      reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Create a class include four constructors with annotation or without.
 * -#step2: Use classloader to load class.
 * -#step3: Get constructors by getConstructor() and getDeclaredConstructor(Class...<?> parameterTypes) and
 *          getConstructor(Class<?>... parameterTypes).
 * -#step4: Constructor object get the get the annotation of the target class's constructor by calling
 *          getAnnotation(Class<T> annotationClass)
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTConstructorGetAnnotation1.java
 *- @ExecuteClass: RTConstructorGetAnnotation1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Constructor;

@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Www1 {
    int i() default 0;

    String t() default "";
}

@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Www1_a {
    int c() default 0;

    String d() default "";
}

class ConstructorGetAnnotation1 {
    @Www1(i = 333, t = "ConstructorgetAnnotation")
    public ConstructorGetAnnotation1() {
    }

    public ConstructorGetAnnotation1(String name) {
    }

    @Www1_a(c = 666, d = "Constructor")
    ConstructorGetAnnotation1(int number) {
    }

    ConstructorGetAnnotation1(String name, int number) {
    }
}

public class RTConstructorGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class zqp = Class.forName("ConstructorGetAnnotation1");
            Constructor zhu1 = zqp.getDeclaredConstructor(int.class);
            Constructor zhu2 = zqp.getConstructor();
            Constructor zhu3 = zqp.getConstructor(String.class);
            if ((zhu1.getAnnotation(Www1_a.class).toString().indexOf("c=666") != -1 &&
                    zhu1.getAnnotation(Www1_a.class).toString().indexOf("d=Constructor") != -1 &&
                    zhu2.getAnnotation(Www1.class).toString().indexOf("i=333") != -1 &&
                    zhu2.getAnnotation(Www1.class).toString().indexOf("t=ConstructorgetAnnotation") != -1)) {
                if (zhu3.getAnnotation(Www1.class) == null) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
