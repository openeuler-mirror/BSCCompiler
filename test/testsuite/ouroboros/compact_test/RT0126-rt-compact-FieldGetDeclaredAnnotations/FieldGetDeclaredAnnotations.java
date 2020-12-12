/*
 * - @TestCaseID: FieldGetDeclaredAnnotations.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:FieldGetDeclaredAnnotations.java
 * - @Title/Destination: Field.getDeclaredAnnotation returns annotations that are directly present on this element.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest08。
 *  -#step2：通过调用getField()从内部类MyTargetTest08中获取home。
 *  -#step3：调用getDeclaredAnnotation(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step4：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: FieldGetDeclaredAnnotations.java
 * - @ExecuteClass: FieldGetDeclaredAnnotations
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Field;

public class FieldGetDeclaredAnnotations {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = MethodGetDeclaredAnnotations1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int MethodGetDeclaredAnnotations1() {
        Field m;
        try {
            m = MyTargetTest08.class.getField("home");
            MyTarget Target = m.getDeclaredAnnotation(MyTarget.class);
            if ("@FieldGetDeclaredAnnotations$MyTarget(name=newName, value=newValue)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest08 {
        @MyTarget(name = "newName", value = "newValue")
        public String home;

        @MyTarget(name = "name", value = "value")
        public void MyTargetTest_1() {
            System.out.println("This is Example:hello world");
        }

        public void newMethod(@MyTarget(name = "name1", value = "value1") String home) {
            System.out.println("my home at:" + home);
        }

        @MyTarget(name = "cons", value = "constructor")
        public MyTargetTest08(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
