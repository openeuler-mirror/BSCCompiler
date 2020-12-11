/*
 * - @TestCaseID: ParameterGetDeclaredAnnotationsByType.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ParameterGetDeclaredAnnotationsByType.java
 * - @Title/Destination: Parameter.getDeclaredAnnotationsByType(Class<T> annotationClass) Returns this element's
 *                       annotation(s) for the specified type if such annotations are either directly present or
 *                       indirectly present.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest22。
 *  -#step2：通过调用getMethod()从内部类MyTargetTest22中获取newMethod。
 *  -#step3：调用getParameters()获取所有的参数。
 *  -#step4：调用getDeclaredAnnotationsByType(Class<T> annotationClass)获取类型为MyTarget的注解。
 *  -#step5：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ParameterGetDeclaredAnnotationsByType.java
 * - @ExecuteClass: ParameterGetDeclaredAnnotationsByType
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;

public class ParameterGetDeclaredAnnotationsByType {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ParameterGetDeclaredAnnotationsByType1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ParameterGetDeclaredAnnotationsByType1() {
        Method m;
        try {
            m = MyTargetTest23.class.getMethod("newMethod", new Class[] {String.class});
            Parameter[] parameters = m.getParameters();
            MyTarget[] Target = parameters[0].getDeclaredAnnotationsByType(MyTarget.class);
            if ("@ParameterGetDeclaredAnnotationsByType$MyTarget(name=name1, value=value1)".equals(Target[0].toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest23 {
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
        public MyTargetTest23(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
