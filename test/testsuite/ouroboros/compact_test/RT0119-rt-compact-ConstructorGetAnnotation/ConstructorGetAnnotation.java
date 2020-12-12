/*
 * - @TestCaseID: ConstructorGetAnnotation.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ConstructorGetAnnotation.java
 * - @Title/Destination: Constructor.getAnnotation() return the annotation as expected.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest00。
 *  -#step2：通过调用getConstructor(Class[])从内部类MyTargetTest00中获取对应的构造方法。
 *  -#step3：调用getAnnotation(Class<T> annotationClass)从构造方法中获取类型为MyTarget的注解。
 *  -#step4：确认获取的注解正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ConstructorGetAnnotation.java
 * - @ExecuteClass: ConstructorGetAnnotation
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;

public class ConstructorGetAnnotation {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;

        try {
            result = ConstructorAnnotation1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ConstructorAnnotation1() {
        Constructor<?> m;
        try {
            m = MyTargetTest00.class.getConstructor(new Class[] {ConstructorGetAnnotation.class, String.class});
            MyTarget Target = m.getAnnotation(MyTarget.class);
            if ("@ConstructorGetAnnotation$MyTarget(name=cons, value=constructor)".equals(Target.toString())) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest00 {
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
        public MyTargetTest00(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
