/*
 *- @TestCaseID: ReflectionGetAnnotation6
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotation6.java
 *- @Title/Destination: Call Class.GetAnnotation() to get annotations of inner classes by reflection
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 定义含注解的类GetAnnotation6的静态内部类GetAnnotation6_a。
 *  -#step2: 通过forName()获取GetAnnotation6_a的类名对象。
 *  -#step3：调用getAnnotation(Class<T> annotationClass)获取GetAnnotation6_b的注解。
 *  -#step4：对获取的注解结果进行判断
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotation6.java
 *- @ExecuteClass: ReflectionGetAnnotation6
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz6 {
    int i() default 0;

    String t() default "";
}

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz6_a {
    int i_a() default 2;

    String t_a() default "";
}

@Zzz6(i = 333, t = "test1")
class GetAnnotation6 {
    public int i;
    public String t;

    @Zzz6_a(i_a = 666, t_a = "right1")
    public static class GetAnnotation6_a {
        public int j;
    }
}

public class ReflectionGetAnnotation6 {

    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation6");
            Class[] zhu1 = zqp1.getDeclaredClasses();
            if (zhu1[0].getAnnotation(Zzz6_a.class).toString().indexOf("t_a=right1") != -1 && zhu1[0].
                    getAnnotation(Zzz6_a.class).toString().indexOf("i_a=666") != -1) {
                if (zhu1[0].getAnnotation(Zzz6.class) == null) {
                    System.out.println(0);
                }
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
