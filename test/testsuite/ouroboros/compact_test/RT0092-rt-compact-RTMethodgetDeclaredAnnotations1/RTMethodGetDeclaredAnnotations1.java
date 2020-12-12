/*
 *- @TestCaseID: RTMethodGetDeclaredAnnotations1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTMethodGetDeclaredAnnotations1.java
 *- @Title/Destination: Method.getDeclaredAnnotations() returns annotations that are directly present on this element.
 *                      Ignores inherited annotations.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取MethodGetDeclaredAnnotations1_a类的运行时类clazz；
 * -#step2: 以ii、String.class为参数，通过getMethod()方法获取clazz的方法对象并记为method1；
 * -#step3: 通过判断得出method1.getDeclaredAnnotations().length的返回值为1，并通过getDeclaredAnnotations()方法获取
 *          method1的所有被声明的注解并记为annotations1；
 * -#step4: 经过判断得出annotations1[0].toString()包含字符串i_a=666和t_a=right1；
 * -#step5: 以tt、int.class为参数，通过getDeclaredMethod()方法获取clazz的声明方法并记为method2；
 * -#step6: 通过判断得出method2.getDeclaredAnnotations().length的返回值为1，并通过getDeclaredAnnotations()方法获取
 *          method2的所有被声明的注解并记为annotations2；
 * -#step7: 经过判断得出annotations2[0].toString()包含字符串i_a=666和t_a=right1；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTMethodGetDeclaredAnnotations1.java
 *- @ExecuteClass: RTMethodGetDeclaredAnnotations1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Method;

@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IFw1 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IFw1_a {
    int i_a() default 2;
    String t_a() default "";
}

class MethodGetDeclaredAnnotations1 {
    @IFw1(i = 333, t = "test1")
    public static void ii(String name) {
    }

    @IFw1(i = 333, t = "test1")
    void tt(int number) {
    }
}

class MethodGetDeclaredAnnotations1_a extends MethodGetDeclaredAnnotations1 {
    @IFw1_a(i_a = 666, t_a = "right1")
    public static void ii(String name) {
    }

    @IFw1_a(i_a = 666, t_a = "right1")
    void tt(int number) {
    }
}

public class RTMethodGetDeclaredAnnotations1 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetDeclaredAnnotations1_a");
            Method method1 = clazz.getMethod("ii", String.class);
            if (method1.getDeclaredAnnotations().length == 1) {
                Annotation[] annotations1 = method1.getDeclaredAnnotations();
                if (annotations1[0].toString().indexOf("i_a=666") != -1
                        && annotations1[0].toString().indexOf("t_a=right1") != -1) {
                    Method method2 = clazz.getDeclaredMethod("tt", int.class);
                    if (method2.getDeclaredAnnotations().length == 1) {
                        Annotation[] annotations2 = method2.getDeclaredAnnotations();
                        if (annotations2[0].toString().indexOf("i_a=666") != -1
                                && annotations2[0].toString().indexOf("t_a=right1") != -1) {
                            System.out.println(0);
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
