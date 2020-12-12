/*
 * - @TestCaseID: ParameterGetAnnotations
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ParameterGetAnnotations.java
 * - @Title/Destination: Parameter.getAnnotations() Returns annotations that are present on this element.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest19。
 *  -#step2：通过调用getMethod()从内部类MyTargetTest19中获取newMethod。
 *  -#step3：调用getParameters()获取所有的参数。
 *  -#step4：调用getAnnotations()获取类型为MyTarget的注解。
 *  -#step5：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ParameterGetAnnotations.java
 * - @ExecuteClass: ParameterGetAnnotations
 * - @ExecuteArgs:
 */

import java.lang.reflect.Method;
import java.lang.reflect.Parameter;
import java.lang.annotation.Annotation;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class ParameterGetAnnotations {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ParameterGetAnnotations1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ParameterGetAnnotations1() {
        Method m;
        try {
            m = MyTargetTest19.class.getMethod("newMethod", new Class[] {String.class});
            Parameter[] parameters = m.getParameters();
            Annotation[] Target = parameters[0].getAnnotations();
            if ("@ParameterGetAnnotations$MyTarget(name=name1, value=value1)".equals(Target[0].toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest19 {
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
        public MyTargetTest19(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
