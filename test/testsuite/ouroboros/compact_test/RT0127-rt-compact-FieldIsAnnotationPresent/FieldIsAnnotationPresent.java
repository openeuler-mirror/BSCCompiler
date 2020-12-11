/*
 * - @TestCaseID: FieldIsAnnotationPresent.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:FieldIsAnnotationPresent.java
 * - @Title/Destination: Field.isAnnotationPresent() Returns true if an annotation for the specified type is present on this element, else false
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest09。
 *  -#step2：通过调用getField()从内部类MyTargetTest08中获取home。
 *  -#step3：调用isAnnotationPresent(Class<T> annotationClass)判断是否存在类型为MyTarget的注解。
 *  -#step4：确认返回true。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: FieldIsAnnotationPresent.java
 * - @ExecuteClass: FieldIsAnnotationPresent
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Field;

public class FieldIsAnnotationPresent {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = MethodIsAnnotationPresent1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int MethodIsAnnotationPresent1() {
        Field m;
        try {
            m = MyTargetTest09.class.getField("home");
            boolean flag = m.isAnnotationPresent(MyTarget.class);
            if (flag) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest09 {
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
        public MyTargetTest09(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
