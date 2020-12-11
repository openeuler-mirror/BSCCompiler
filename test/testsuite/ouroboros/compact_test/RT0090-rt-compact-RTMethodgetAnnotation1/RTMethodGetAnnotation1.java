/*
 *- @TestCaseID: RTMethodGetAnnotation1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTMethodGetAnnotation1.java
 *- @Title/Destination: Method.getAnnotation() Returns the Method's annotation for the specified type if such an
 *                      annotation is present, else null.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取MethodGetAnnotation1类的运行时类clazz；
 * -#step2: 以test3、int.class为参数，通过getDeclaredMethod()方法获取clazz的声明方法并记为method1；
 * -#step3: 分别以test1、（test2、String.class）为参数，通过getMethod()方法获取clazz的方法对象method2、method3；
 * -#step4: 分别以IF1_a.class、IF1.class为参数，通过getAnnotation()方法获取method1、method2的注解，且因为存在这样的
 *          注解，因此可以返回成功；
 * -#step5: 以IF1.class为参数，通过getAnnotation()方法获取method3的注解，因不存在这样的注解，因此返回为null；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTMethodGetAnnotation1.java
 *- @ExecuteClass: RTMethodGetAnnotation1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Method;

@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i_a() default 0;
    String t_b() default "";
}

@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    int c() default 0;
    String d() default "";
}

class MethodGetAnnotation1 {
    @IF1(i_a = 333, t_b = "MethodGetAnnotation")
    public void test1() {
    }

    public void test2(String name) {
    }

    @IF1_a(c = 666, d = "Method")
    void test3(int number) {
    }

    void test4(String name, int number) {
    }
}

public class RTMethodGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetAnnotation1");
            Method method1 = clazz.getDeclaredMethod("test3", int.class);
            Method method2 = clazz.getMethod("test1");
            Method method3 = clazz.getMethod("test2", String.class);
            if (method1.getAnnotation(IF1_a.class).c() == 666
                    && method1.getAnnotation(IF1_a.class).d().equals("Method")
                    && method2.getAnnotation(IF1.class).i_a() == 333
                    && method2.getAnnotation(IF1.class).t_b().equals("MethodGetAnnotation")) {
                if (method3.getAnnotation(IF1.class) == null) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
