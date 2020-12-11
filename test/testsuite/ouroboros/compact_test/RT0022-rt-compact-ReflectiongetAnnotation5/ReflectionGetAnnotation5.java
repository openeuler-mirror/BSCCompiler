/*
 *- @TestCaseID: ReflectionGetAnnotation5
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotation5.java
 *- @Title/Destination: Child class call Class.GetAnnotation to get class annotations inherited from parent classes
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 定义含注解的内部类GetAnnotation5。
 *  -#step2：创建类GetAnnotation5_a继承GetAnnotation5，创建类GetAnnotation5_a的子类GetAnnotation5_b。
 *  -#step3: 通过forName()获取GetAnnotation5_b的类名对象。
 *  -#step4：调用getAnnotation(Class<T> annotationClass)获取GetAnnotation5_b的注解。
 *  -#step5：对获取的注解结果进行判断
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotation5.java
 *- @ExecuteClass: ReflectionGetAnnotation5
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz5 {
    int i() default 0;

    String t() default "";
}

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz5_a {
    int i_a() default 2;

    String t_a() default "";
}

@Zzz5(i = 333, t = "test1")
class GetAnnotation5 {
    public int i;
    public String t;
}

@Zzz5_a(i_a = 666, t_a = "right1")
class GetAnnotation5_a extends GetAnnotation5 {
    public int i_a;
    public String t_a;
}

class GetAnnotation5_b extends GetAnnotation5_a {
}

public class ReflectionGetAnnotation5 {

    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation5_b");
            if (zqp1.getAnnotation(Zzz5.class).toString().indexOf("t=test1") != -1 && zqp1.getAnnotation(Zzz5.class).
                    toString().indexOf("i=333") != -1) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
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
