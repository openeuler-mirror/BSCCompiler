/*
 * - @TestCaseID: MethodGetDeclaredAnnotations
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:MethodGetDeclaredAnnotations.java
 * - @Title/Destination: Method.getDeclaredAnnotation() returns this element's annotation for the specified type if such
 *                       an annotation is directly present。
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest12。
 *  -#step2：通过调用getMethod()从内部类MyTargetTest12中获取MyTargetTest_1。
 *  -#step3：调用getDeclaredAnnotation(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step4：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: MethodGetDeclaredAnnotations.java
 * - @ExecuteClass: MethodGetDeclaredAnnotations
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;

public class MethodGetDeclaredAnnotations {
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
        Method m;
        try {
            m = MyTargetTest12.class.getMethod("MyTargetTest_1");
            MyTarget Target = m.getDeclaredAnnotation(MyTarget.class);
            if ("@MethodGetDeclaredAnnotations$MyTarget(name=name, value=value)".equals(Target.toString())) {
                return 0;
            }
        } catch (NoSuchMethodException | SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest12 {
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
        public MyTargetTest12(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
