/*
 *- @TestCaseID: RTParameterGetAnnotation1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTParameterGetAnnotation1.java
 *- @Title/Destination: Parameter.getAnnotation() returns the method parameter's annotations for the specified type if
 *                      such an annotation is present, else null.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 自定义一个类，含有注解的构造方法，不含注解的构造方法和有注解的方法。
 * -#step2：通过调用getDeclaredConstructor获取自定义类的构造方法，通过调用getParameters()获取参数数组，调用
 *          getAnnotation(Class<T> annotationClass)获取注解，确认获取成功。
 * -#step3：通过调用getMethod获取自定义类的方法，通过调用getParameters()获取参数数组，调用
 *          getAnnotation(Class<T> annotationClass)获取注解，确认获取成功。
 * -#step4：通过调用getDeclaredConstructor获取自定义类的不含注解的构造方法，通过调用getParameters()获取参数数组，调用
 *          getAnnotation(Class<T> annotationClass)获取注解，确认获取为空。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTParameterGetAnnotation1.java
 *- @ExecuteClass: RTParameterGetAnnotation1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;

@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    int c() default 0;
    String d() default "";
}

class ParameterGetAnnotation1 {
    ParameterGetAnnotation1(@IF1(i = 222, t = "Parameter") int number) {
    }

    public ParameterGetAnnotation1(String name) {
    }

    public void test1(@IF1_a(c = 666, d = "Happy new year") int age) {
    }
}

public class RTParameterGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetAnnotation1");
            Constructor cons1 = cls.getDeclaredConstructor(int.class);
            Constructor cons2 = cls.getConstructor(String.class);
            Method method = cls.getMethod("test1", int.class);
            Parameter[] p1 = cons1.getParameters();
            Parameter[] p2 = cons2.getParameters();
            Parameter[] p3 = method.getParameters();
            if (p1[0].getAnnotation(IF1.class).i() == 222 && p1[0].getAnnotation(IF1.class).t().equals("Parameter")
                    && p3[0].getAnnotation(IF1_a.class).c() == 666
                    && p3[0].getAnnotation(IF1_a.class).d().equals("Happy new year")) {
                if (p2[0].getAnnotation(IF1.class) == null) {
                    System.out.println(0);
                    return;
                }
                System.out.println(2);
                return;
            }
        } catch (ClassNotFoundException e) {
            System.out.println(3);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(4);
            return;
        }
        System.out.println(5);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
