/*
 *- @TestCaseID: RTParameterGetDeclaredAnnotations1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTParameterGetDeclaredAnnotations1.java
 *- @Title/Destination: Parameter.getDeclaredAnnotations() returns annotations that are directly present on this element.
 *                      This method ignores inherited annotations.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 自定义一个类，含有注解的构造方法和不含注解的方法。
 * -#step2：通过调用getDeclaredConstructor获取自定义类的构造方法，通过调用getParameters()获取参数数组，调用
 *          getDeclaredAnnotations()获取注解数组，检查数组长度为2。
 * -#step3：通过调用getMethod获取自定义类的方法，通过调用getParameters()获取参数数组，调用getDeclaredAnnotations()获取注
 *          解数组，检查数组长度为0。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTParameterGetDeclaredAnnotations1.java
 *- @ExecuteClass: RTParameterGetDeclaredAnnotations1
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
@interface IF5 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF5_a {
    int c() default 0;
    String d() default "";
}

class ParameterGetDeclaredAnnotations1 {
    ParameterGetDeclaredAnnotations1(@IF5(i = 222, t = "Parameter") @IF5_a(c = 666, d = "Happy new year") int number) {
    }

    public void test1(String name) {
    }
}

public class RTParameterGetDeclaredAnnotations1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetDeclaredAnnotations1");
            Constructor cons = cls.getDeclaredConstructor(int.class);
            Method method = cls.getMethod("test1", String.class);
            Parameter[] p1 = cons.getParameters();
            Parameter[] p2 = method.getParameters();
            if (p1[0].getDeclaredAnnotations().length == 2 && p2[0].getDeclaredAnnotations().length == 0) {
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
