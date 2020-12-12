/*
 *- @TestCaseID: RTFieldGetDeclaredAnnotations2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTFieldGetDeclaredAnnotations2.java
 *- @Title/Destination: Field.GetDeclaredAnnotations() ignores inherited annotations.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义含注解类FieldGetDeclaredAnnotations2，定义含注解的类FieldGetDeclaredAnnotations2的子类
 *          FieldGetDeclaredAnnotations2_a。
 * -#step2: 通过调用forName()方法加载类FieldGetDeclaredAnnotations2_a。
 * -#step3: 通过调用 getField()获取对应名称的成员变量。
 * -#step4: 通过调用getAnnotation(Class<T> annotationClass)获取注解，调用equals()方法进行判断获取正确。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTFieldGetDeclaredAnnotations2.java
 *- @ExecuteClass: RTFieldGetDeclaredAnnotations2
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Field;

@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface interface2 {
    int num() default 0;
    String str() default "";
}

@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface interface2_a {
    int i_a() default 2;
    String t_a() default "";
}

class FieldGetDeclaredAnnotations2 {
    @interface2(num = 333, str = "test1")
    public int num;
    @interface2(num = 333, str = "test1")
    public String str;
}

class FieldGetDeclaredAnnotations2_a extends FieldGetDeclaredAnnotations2 {
    @interface2_a(i_a = 666, t_a = "right1")
    public int num;
    public String str;
}

public class RTFieldGetDeclaredAnnotations2 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetDeclaredAnnotations2_a");
            Field instance1 = cls.getField("str");
            Field instance2 = cls.getDeclaredField("str");
            if (instance1.getDeclaredAnnotations().length == 0 && instance2.getDeclaredAnnotations().length == 0) {
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
