/*
 *- @TestCaseID: ReflectionGetDeclaredAnnotationsByTypeNullPointerException
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredAnnotationsByTypeNullPointerException.java
 *- @Title/Destination: Class.GetDeclaredAnnotationsByType(null) throws NullPointerException.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Define two annotation.
 * -#step2: Use classloader to load class.
 * -#step3: Test Class.GetDeclaredAnnotationsByType() with null parameter.
 * -#step4: Check that NullPointerException was threw.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredAnnotationsByTypeNullPointerException.java
 *- @ExecuteClass: ReflectionGetDeclaredAnnotationsByTypeNullPointerException
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Dddd2 {
    int i() default 0;

    String t() default "";
}

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Dddd2_a {
    int i_a() default 2;

    String t_a() default "";
}

@Dddd2(i = 333, t = "test1")
class GetDeclaredAnnotationsByType2 {
    public int i;
    public String t;
}

@Dddd2_a(i_a = 666, t_a = "right1")
class GetDeclaredAnnotationsByType2_a extends GetDeclaredAnnotationsByType2 {
    public int i_a;
    public String t_a;
}

class GetDeclaredAnnotationsByType2_b extends GetDeclaredAnnotationsByType2_a {
}

public class ReflectionGetDeclaredAnnotationsByTypeNullPointerException {

    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        try {
            Class zqp1 = Class.forName("GetDeclaredAnnotationsByType2_b");
            Annotation[] j = zqp1.getDeclaredAnnotationsByType(null);
            result = 0;
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        } catch (NullPointerException e2) {
            result = 0;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
