/*
 *- @TestCaseID: RTParameterGetAnnotations1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTParameterGetAnnotations1.java
 *- @Title/Destination: Get all annotations of target class method parameters by reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 自定义一个类，含有注解的构造方法，和不含注解的方法。
 * -#step2：通过调用getDeclaredConstructor获取自定义类的构造方法，通过调用getParameters()获取参数数组，调用getAnnotations()获
 *          取注解数组，确认注解数组长度为2。
 * -#step3：通过调用getMethod获取自定义类的方法，通过调用getParameters()获取参数数组，调用getAnnotations()获取注解数组，确认注
 *          解数组长度为0。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTParameterGetAnnotations1.java
 *- @ExecuteClass: RTParameterGetAnnotations1
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
@interface IF3 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF3_a {
    int c() default 0;
    String d() default "";
}

class ParameterGetAnnotations1 {
    ParameterGetAnnotations1(@IF3(i = 222, t = "Parameter") @IF3_a(c = 666, d = "Happy new year") int number) {
    }

    public void test1(String name) {
    }
}

public class RTParameterGetAnnotations1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetAnnotations1");
            Constructor cons = cls.getDeclaredConstructor(int.class);
            Method method = cls.getMethod("test1", String.class);
            Parameter[] p1 = cons.getParameters();
            Parameter[] p2 = method.getParameters();
            if (p1[0].getAnnotations().length == 2 && p2[0].getAnnotations().length == 0) {
                System.out.println(0);
                return;
            }
        } catch (ClassNotFoundException e) {
            System.out.println(1);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(2);
            return;
        }
        System.out.println(3);
        return;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
