/*
 *- @TestCaseID: ReflectionGetAnnotationsByType1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotationsByType1.java
 *- @Title/Destination: Call Class.GetAnnotationsByType() to gets the annotation of the field in the target class by
 *                      reflection, returning the annotation array.
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 创建类GetAnnotationsByType1_a，且定义含注解的变量。
 *  -#step2: 通过forName()获取GetAnnotationsByType1_a的类名对象。
 *  -#step3: 通过getMethod()获取GetAnnotationsByType1_a内对应名称的变量。
 *  -#step4：调用getAnnotationsByType(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step5：对获取的注解结果进行判断
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotationsByType1.java
 *- @ExecuteClass: ReflectionGetAnnotationsByType1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Field;

@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzzz1 {
    int i() default 0;

    String t() default "";
}

@interface Zzzz1_a {
}

@interface Zzzz1_b {
}

class GetAnnotationsByType1_a {
    @Zzzz1(i = 333, t = "GetAnnotationsByType")
    public int i_a;
    @Zzzz1(i = 333, t = "GetAnnotationsByType")
    public int i_aa;
    @Zzzz1(i = 333, t = "GetAnnotationsByType")
    public int i_aaa;
    public String t_a;
    @Zzzz1_a
    public String t_aa;
    @Zzzz1_b
    public String t_aaa;
}

public class ReflectionGetAnnotationsByType1 {

    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotationsByType1_a");
            Field zhu1 = zqp1.getField("i_a");
            Field zhu2 = zqp1.getField("t_a");
            if (zqp1.getAnnotationsByType(Zzzz1.class).length == 0) {
                if (zhu2.getAnnotationsByType(Zzzz1_a.class).length == 0) {
                    Annotation[] k = zhu1.getAnnotationsByType(Zzzz1.class);
                    if (k[0].toString().indexOf("t=GetAnnotationsByType") != -1 && k[0].toString().indexOf("i=333")
                            != -1) {
                        System.out.println(0);
                    }
                }
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
