/*
 * - @TestCaseID: FieldGetAnnotation.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:FieldGetAnnotation.java
 * - @Title/Destination: Field.getAnnotation() Returns the Field's annotation for the specified type if such an
 *                       annotation is present.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest06。
 *  -#step2：通过调用getField()从内部类MyTargetTest06中获取home。
 *  -#step3：调用getAnnotation(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step4：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: FieldGetAnnotation.java
 * - @ExecuteClass: FieldGetAnnotation
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Field;

public class FieldGetAnnotation {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = FieldGetAnnotation1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int FieldGetAnnotation1() {
        Field m;
        try {
            m = MyTargetTest06.class.getField("home");
            MyTarget Target = m.getAnnotation(MyTarget.class);
            if ("@FieldGetAnnotation$MyTarget(name=newName, value=newValue)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest06 {
        @MyTarget(name = "newName", value = "newValue")
        public String home;

        @MyTarget(name = "name", value = "value")
        public void MyTargetTest_1() {
            System.out.println("This is Example:hello world");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
