/*
 *- @TestCaseID: ReflectionGetAnnotationsByType3
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotationsByType3.java
 *- @Title/Destination: Child class calls Class.GetAnnotationsByType to gets the annotations inherited from parent class
  *                     by reflection, and returns an array of annotations.
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 创建类GetAnnotationsByType3_a继承于含注解的GetAnnotationsByType3的子类GetAnnotationsByType3_a。
 *  -#step2: 通过forName()获取GetAnnotationsByType1_a的类名对象。
 *  -#step3：调用getAnnotationsByType(Class<T> annotationClass)获取类型为MyTarget的数组。
 *  -#step5：对获取的注解结果进行判断。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotationsByType3.java
 *- @ExecuteClass: ReflectionGetAnnotationsByType3
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzzz3 {
    int i() default 0;

    String t() default "";
}

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzzz3_a {
    int i_a() default 2;

    String t_a() default "";
}

@Zzzz3(i = 333, t = "test1")
class GetAnnotationsByType3 {
    public int i;
    public String t;
}

@Zzzz3_a(i_a = 666, t_a = "right1")
class GetAnnotationsByType3_a extends GetAnnotationsByType3 {
    public int i_a;
    public String t_a;
}

class GetAnnotationsByType3_b extends GetAnnotationsByType3_a {
}

public class ReflectionGetAnnotationsByType3 {

    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotationsByType3_b");
            Annotation[] j = zqp1.getAnnotationsByType(Zzzz3.class);
            if (j[0].toString().indexOf("t=test1") != -1 && j[0].toString().indexOf("i=333") != -1) {
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
