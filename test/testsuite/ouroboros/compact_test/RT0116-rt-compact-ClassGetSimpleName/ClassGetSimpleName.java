/*
 * - @TestCaseID: ClassGetSimpleName
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassGetSimpleName.java
 * - @Title/Destination: Class.getSimpleName() return Returns the simple name of the underlying class as given in the
 *                       source code。
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest4。
 *  -#step2：获取class MyTargetTest4。
 *  -#step3：调用getSimpleName()获取底层类的简称。
 *  -#step4：确认返回的名称正确。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ClassGetSimpleName.java
 * - @ExecuteClass: ClassGetSimpleName
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class ClassGetSimpleName {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ClassGetSimpleName1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ClassGetSimpleName1() {
        Class<MyTargetTest4> m;
        try {
            m = MyTargetTest4.class;
            String str = m.getSimpleName();
            if ("MyTargetTest4".equals(str)) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest4 {
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
        public MyTargetTest4(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
