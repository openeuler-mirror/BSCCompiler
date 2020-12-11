/*
 * - @TestCaseID: FieldGetAnnotationsByType.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:FieldGetAnnotationsByType.java
 * - @Title/Destination: Field.getAnnotationsByType() returns annotations that are associated with this element.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest07。
 *  -#step2：通过调用getField()从内部类MyTargetTest07中获取home。
 *  -#step3：调用getAnnotationsByType(Class<T> annotationClass)获取类型为MyTarget的注解数组。
 *  -#step4：确认获取的注解数组个数大于0。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: FieldGetAnnotationsByType.java
 * - @ExecuteClass: FieldGetAnnotationsByType
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Field;

public class FieldGetAnnotationsByType {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = FieldGetAnnotationsByType1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int FieldGetAnnotationsByType1() {
        Field m;
        try {
            m = MyTargetTest07.class.getField("home");
            MyTarget[] Target = m.getAnnotationsByType(MyTarget.class);
            if (Target.length > 0) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest07 {
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
        public MyTargetTest07(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
