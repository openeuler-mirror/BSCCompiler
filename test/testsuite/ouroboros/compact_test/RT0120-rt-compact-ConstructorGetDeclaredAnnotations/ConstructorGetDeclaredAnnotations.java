/*
 * - @TestCaseID: ConstructorGetDeclaredAnnotations.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ConstructorGetDeclaredAnnotations.java
 * - @Title/Destination: Constructor.getDeclaredAnnotation() returns this element's annotation for the specified type if such an annotation is directly present.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest01。
 *  -#step2：通过调用getConstructor(Class[])从内部类MyTargetTest01中获取对应的构造方法。
 *  -#step3：调用getDeclaredAnnotation(Class<T> annotationClass)从构造方法中获取类型为MyTarget的注解。
 *  -#step4：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ConstructorGetDeclaredAnnotations.java
 * - @ExecuteClass: ConstructorGetDeclaredAnnotations
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;

public class ConstructorGetDeclaredAnnotations {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ConstructorGetDeclaredAnnotations1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ConstructorGetDeclaredAnnotations1() {
        Constructor<MyTargetTest01> m;
        try {
            m = MyTargetTest01.class.getConstructor(new Class[] {ConstructorGetDeclaredAnnotations.class, String.class});
            MyTarget Target = m.getDeclaredAnnotation(MyTarget.class);
            if ("@ConstructorGetDeclaredAnnotations$MyTarget(name=cons, value=constructor)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest01 {
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
        public MyTargetTest01(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
