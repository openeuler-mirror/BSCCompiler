/*
 * - @TestCaseID: ClassGetEnclosingClass
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassGetEnclosingClass.java
 * - @Title/Destination: Class.getEnclosingClass() Returns the immediately enclosing class of the underlying class.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest2。
 *  -#step2：获取class MyTargetTest2。
 *  -#step3：调用getEnclosingClass()获取底层类的立即封闭类。
 *  -#step4：确认返回不为空。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ClassGetEnclosingClass.java
 * - @ExecuteClass: ClassGetEnclosingClass
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class ClassGetEnclosingClass {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ClassGetEnclosingClass1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ClassGetEnclosingClass1() {
        Class<MyTargetTest2> m;
        try {
            m = MyTargetTest2.class;
            Class<?> cl = m.getEnclosingClass();
            if (cl != null) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest2 {
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
        public MyTargetTest2(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
