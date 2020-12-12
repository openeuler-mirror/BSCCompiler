/*
 *- @TestCaseID: ReflectionGetAnnotation2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotation2.java
 *- @Title/Destination: Call class.getAnnotation to get annotation of a field in class according to annotation class by
 *                      reflection.
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 定义含注解的类GetAnnotation2_a，通过调用getField()获取注解成员，调用getAnnotation(Class<T> annotationClass)
 *           获取注解，确认获取的注解正确。
 *  -#step2：GetAnnotation2_a通过调用getField()获取非注解成员，调用getAnnotation(Class<T> annotationClass)
 *           获取注解，确认获取的注解为空。
 *  -#step3：定义不含注解的类GetAnnotation2_b，通过调用getField()获取成员，调用getAnnotation(Class<T> annotationClass)
 *           获取注解，确认获取的注解为空。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotation2.java
 *- @ExecuteClass: ReflectionGetAnnotation2
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Field;

@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF2 {
    int i() default 0;
    String t() default "";
}

class GetAnnotation2_a {
    @IF2(i = 333, t = "getAnnotation")
    public int i_a;
    public String t_a;
}

class GetAnnotation2_b {
    public int i_b;
    public String t_b;
}

public class ReflectionGetAnnotation2 {
    public static void main(String[] args) {
        try {
            Class cls1 = Class.forName("GetAnnotation2_a");
            Class cls2 = Class.forName("GetAnnotation2_b");
            Field instance1 = cls1.getField("i_a");
            Field instance2 = cls1.getField("t_a");
            Field instance3 = cls2.getField("i_b");
            if (instance1.getAnnotation(IF2.class).i() == 333 && instance1.getAnnotation(IF2.class).
                    t().equals("getAnnotation") && instance2.getAnnotation(IF2.class) == null &&
                    instance3.getAnnotation(IF2.class) == null) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchFieldException e1) {
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
