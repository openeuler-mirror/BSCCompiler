/*
 *- @TestCaseID: RTParameterGetDeclaredAnnotation1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTParameterGetDeclaredAnnotation1.java
 *- @Title/Destination: Parameter.getDeclaredAnnotation() Returns this element's annotation for the specified type if
 *                     such an annotation is directly present, else null. This method ignores inherited annotations.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 自定义一个类，含有注解的构造方法，不含注解的构造方法和含注解的方法。
 * -#step2：通过调用getDeclaredConstructor获取有注解的构造方法，通过调用getParameters()获取参数数组，调用
 *          getDeclaredAnnotation(Class<T> annotationClass)获取注解，annotationClass为对应的注解类型，确认获取的注解内容
 *          正确。
 * -#step3：通过调用getMethod获取自定义类的方法，通过调用getParameters()获取参数数组，调用
 *          getDeclaredAnnotation(Class<T> annotationClass)获取注解，annotationClass为对应的注解类型，确认获取的注解内容
 *          正确。
 * -#step4：通过调用getDeclaredConstructor获取不含注解的构造方法，通过调用getParameters()获取参数数组，调用
 *          getDeclaredAnnotation(Class<T> annotationClass)获取注解，annotationClass为注解类型，确认返回为空。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTParameterGetDeclaredAnnotation1.java
 *- @ExecuteClass: RTParameterGetDeclaredAnnotation1
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
@interface IF6 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF6_a {
    int c() default 0;
    String d() default "";
}

class ParameterGetDeclaredAnnotation1 {
    ParameterGetDeclaredAnnotation1(@IF6(i = 222, t = "Parameter") int number) {
    }

    public ParameterGetDeclaredAnnotation1(String name) {
    }

    public void test1(@IF6_a(c = 666, d = "Happy new year") int age) {
    }
}

public class RTParameterGetDeclaredAnnotation1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetDeclaredAnnotation1");
            Constructor cons1 = cls.getDeclaredConstructor(int.class);
            Constructor cons2 = cls.getConstructor(String.class);
            Method method = cls.getMethod("test1", int.class);
            Parameter[] p1 = cons1.getParameters();
            Parameter[] p2 = cons2.getParameters();
            Parameter[] p3 = method.getParameters();
            if (p1[0].getDeclaredAnnotation(IF6.class).i() == 222 && p1[0].getDeclaredAnnotation(IF6.class).t()
                    .equals("Parameter") && p3[0].getDeclaredAnnotation(IF6_a.class).c() == 666
                    && p3[0].getDeclaredAnnotation(IF6_a.class).d().equals("Happy new year")) {
                if (p2[0].getDeclaredAnnotation(IF6.class) == null) {
                    System.out.println(0);
                    return;
                }
                System.out.println(1);
                return;
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(3);
            return;
        }
        System.out.println(4);
        return;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
