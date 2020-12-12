/*
 * - @TestCaseID: MethodGetAnnotation
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:MethodGetAnnotation.java
 * - @Title/Destination: Method.getAnnotation Returns this element's annotation for the specified type if such an
 *                       annotation is present, else null.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest11。
 *  -#step2：通过调用getMethod()从内部类MyTargetTest11中获取MyTargetTest_1。
 *  -#step3：调用getAnnotation(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step4：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: MethodGetAnnotation.java
 * - @ExecuteClass: MethodGetAnnotation
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;

public class MethodGetAnnotation {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = MethodGetAnnotation1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int MethodGetAnnotation1() {
        Method m;
        try {
            m = MyTargetTest11.class.getMethod("MyTargetTest_1");
            MyTarget Target = m.getAnnotation(MyTarget.class);
            if ("@MethodGetAnnotation$MyTarget(name=name, value=value)".equals(Target.toString())) {
                return 0;
            }
        } catch (NoSuchMethodException | SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest11 {
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
