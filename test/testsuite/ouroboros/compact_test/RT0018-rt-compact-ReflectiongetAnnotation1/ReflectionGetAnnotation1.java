/*
 *- @TestCaseID: ReflectionGetAnnotation1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotation1.java
 *- @Title/Destination: call class.getAnnotation to get annotation of a target class according to annotation class by
 *                      reflection
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取GetAnnotation1_a类的类型clazz1，通过Class.forName()方法获取GetAnnotation1_b类的
 *          类型clazz2；
 * -#step2: 以Zzz1.class为参数，通过getAnnotation()方法，分别获取clazz1、clazz2的注解，并且clazz1的注解中包含字符串
 *          "t=getAnnotation"和"i=333"，而clazz2的注解等于null；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotation1.java
 *- @ExecuteClass: ReflectionGetAnnotation1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz1 {
int i() default 0;

String t() default "";
}

@Zzz1(i = 333, t = "getAnnotation")
class GetAnnotation1_a {
}

class GetAnnotation1_b {
}

public class ReflectionGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetAnnotation1_a");
            Class clazz2 = Class.forName("GetAnnotation1_b");
            if (clazz1.getAnnotation(Zzz1.class).toString().indexOf("t=getAnnotation") != -1
                    && clazz1.getAnnotation(Zzz1.class).toString().indexOf("i=333") != -1
                    && clazz2.getAnnotation(Zzz1.class) == null) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
