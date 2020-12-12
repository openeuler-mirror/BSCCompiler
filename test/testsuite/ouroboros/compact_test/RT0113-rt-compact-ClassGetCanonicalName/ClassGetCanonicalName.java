/*
 * - @TestCaseID: ClassGetCanonicalName
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassGetCanonicalName.java
 * - @Title/Destination: Class.getCanonicalName() Returns the canonical name of the underlying class as defined by the
 *                       Java Language Specification.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest1。
 *  -#step2：获取class MyTargetTest1。
 *  -#step3：调用getCanonicalName()获取所定义的底层类的规范化名称。
 *  -#step4：确认返回的规范化名称正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ClassGetCanonicalName.java
 * - @ExecuteClass: ClassGetCanonicalName
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class ClassGetCanonicalName {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ClassGetCanonicalName1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ClassGetCanonicalName1() {
        Class<MyTargetTest1> m;
        try {
            m = MyTargetTest1.class;
            String str = m.getCanonicalName();
            if ("ClassGetCanonicalName.MyTargetTest1".equals(str)) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest1 {
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
        public MyTargetTest1(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
