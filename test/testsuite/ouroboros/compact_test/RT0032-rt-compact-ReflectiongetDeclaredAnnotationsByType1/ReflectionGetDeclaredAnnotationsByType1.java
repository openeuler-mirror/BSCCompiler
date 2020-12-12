/*
 *- @TestCaseID: ReflectionGetDeclaredAnnotationsByType1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredAnnotationsByType1.java
 *- @Title/Destination: Class.GetDeclaredAnnotationsByType() returns the target class local annotation of expected type
 *                      through reflection, and returns the annotation array.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Define two annotation.
 * -#step2: Use classloader to load class.
 * -#step3: result an array of annotations by calling GetDeclaredAnnotationsByType().
 * -#step4: Check that Class.GetDeclaredAnnotationsByType() returns all annotations but annotations those inherited from
 *          parent class.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredAnnotationsByType1.java
 *- @ExecuteClass: ReflectionGetDeclaredAnnotationsByType1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Dddd1 {
    int i() default 0;

    String t() default "";
}

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Dddd1_a {
    int i_a() default 2;

    String t_a() default "";
}

@Dddd1(i = 333, t = "test1")
class GetDeclaredAnnotationsByType1 {
    public int i;
    public String t;
}

@Dddd1_a(i_a = 666, t_a = "right1")
class GetDeclaredAnnotationsByType1_a extends GetDeclaredAnnotationsByType1 {
    public int i_a;
    public String t_a;
}

class GetDeclaredAnnotationsByType1_b extends GetDeclaredAnnotationsByType1_a {
}

public class ReflectionGetDeclaredAnnotationsByType1 {

    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        try {
            Class zqp1 = Class.forName("GetDeclaredAnnotationsByType1_b");
            Class zqp2 = Class.forName("GetDeclaredAnnotationsByType1_a");
            if (zqp1.getDeclaredAnnotationsByType(Dddd1.class).length == 0) {
                Annotation[] j = zqp2.getDeclaredAnnotationsByType(Dddd1_a.class);
                if (j[0].toString().indexOf("i_a=666") != -1 && j[0].toString().indexOf("t_a=right1") != -1) {
                    result = 0;
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        } catch (NullPointerException e2) {
            System.err.println(e2);
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
