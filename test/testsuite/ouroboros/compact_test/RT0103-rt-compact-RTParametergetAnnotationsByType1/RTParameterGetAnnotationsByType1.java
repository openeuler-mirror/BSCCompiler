/*
 *- @TestCaseID: RTParameterGetAnnotationsByType1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTParameterGetAnnotationsByType1.java
 *- @Title/Destination: Parameters.getAnnotationsByType returns all this element's annotations for the specified
 *                     annotation type if associated with this element, else an array of length zero。
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 自定义一个类，含有注解的构造方法，不含注解的构造方法，含注解的方法。
 * -#step2：通过调用getConstructor获取自定义类的不含注解的构造方法，通过调用getParameters()获取参数数组，调用
 *          getAnnotationsByType(Class<T> annotationClass)获取注解数组，annotationClass为注解类型，确认注解数组长度为0。
 * -#step3：通过调用getDeclaredConstructor获取自定义类的含注解的构造方法，通过调用getParameters()获取参数数组，调用
 *          getAnnotationsByType(Class<T> annotationClass)获取注解数组，annotationClass为对应的注解类型，对注解数组调用
 *          toString().indexOf(str)判断注解的值是否正确。
 * -#step4：通过调用getMethod获取自定义类的方法，通过调用getParameters()获取参数数组，调用
 *          getAnnotationsByType(Class<T> annotationClass)获取注解数组，annotationClass为对应的注解类型，对注解数组调用
 *          toString().indexOf(str)判断注解的值是否正确。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTParameterGetAnnotationsByType1.java
 *- @ExecuteClass: RTParameterGetAnnotationsByType1
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
@interface IF7 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF7_a {
    int c() default 0;
    String d() default "";
}

class ParameterGetAnnotationsByType1 {
    ParameterGetAnnotationsByType1(@IF7(i = 222, t = "Parameter") int number) {
    }

    public ParameterGetAnnotationsByType1(String name) {
    }

    public void test1(@IF7_a(c = 666, d = "Happy new year") int age) {
    }
}

public class RTParameterGetAnnotationsByType1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetAnnotationsByType1");
            Constructor cons1 = cls.getDeclaredConstructor(int.class);
            Constructor cons2 = cls.getConstructor(String.class);
            Method method = cls.getMethod("test1", int.class);
            Parameter[] p1 = cons1.getParameters();
            Parameter[] p2 = cons2.getParameters();
            Parameter[] p3 = method.getParameters();
            Annotation[] q1 = p1[0].getAnnotationsByType(IF7.class);
            Annotation[] q3 = p3[0].getAnnotationsByType(IF7_a.class);
            if (p2[0].getAnnotationsByType(IF7.class).length == 0) {
                if (q1[0].toString().indexOf("i=222") != -1 && q1[0].toString().indexOf("t=Parameter") != -1
                        && q3[0].toString().indexOf("c=666") != -1
                        && q3[0].toString().indexOf("d=Happy new year") != -1) {
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
