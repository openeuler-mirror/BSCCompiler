/*
 * - @TestCaseID: ParameterGetAnnotation
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ParameterGetAnnotation.java
 * - @Title/Destination: Parameter.getAnnotation() Returns this element's annotation for the specified type if such an
 *                       annotation is present, else null.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest18。
 *  -#step2：通过调用getMethod()从内部类MyTargetTest18中获取newMethod。
 *  -#step3：调用getParameters()获取所有的参数。
 *  -#step4：调用getAnnotation(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step5：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ParameterGetAnnotation.java
 * - @ExecuteClass: ParameterGetAnnotation
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;

public class ParameterGetAnnotation {
    public static void main(String[] args) {
        int result = 2;
        try {
            result = ParameterGetAnnotation1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ParameterGetAnnotation1() {
        Method m;
        try {
            m = MyTargetTest18.class.getMethod("newMethod", new Class[] {String.class});
            Parameter[] parameters = m.getParameters();
            MyTarget Target = parameters[0].getAnnotation(MyTarget.class);
            if ("@ParameterGetAnnotation$MyTarget(name=name1, value=value1)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest18 {
        @MyTarget(name = "newName", value = "newValue")
        public String home;

        @MyTarget(name = "name", value = "value")
        public void MyTargetTest_1() {
            System.out.println("This is Example:hello world");
        }

        public void newMethod(@MyTarget(name = "name1", value = "value1") String home) {
            System.out.println("my home at:" + home);
        }
    }

    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
