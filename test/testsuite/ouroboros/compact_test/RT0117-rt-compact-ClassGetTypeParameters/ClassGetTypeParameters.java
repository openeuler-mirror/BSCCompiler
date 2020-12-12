/*
 * - @TestCaseID: ClassGetTypeParameters
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassGetTypeParameters.java
 * - @Title/Destination: Class.getTypeParameters() return an array of TypeVariable objects that represent the type
 *                       variables declared by the generic declaration represented by this GenericDeclaration object,
 *                       in declaration order.
 * - @Condition: no
 * - @Brief:no:
 *  -#step1: 定义含注解的内部类MyTargetTest5。
 *  -#step2：获取class MyTargetTest5。
 *  -#step3：调用getTypeParameters()获取TypeVariable对象的一个数组。
 *  -#step4：确认返回的数组的长度为0。
 * - @Expect: 0\n
 * - @Priority: High
 * - @Remark:
 * - @Source: ClassGetTypeParameters.java
 * - @ExecuteClass: ClassGetTypeParameters
 * - @ExecuteArgs:
 */

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.TypeVariable;

public class ClassGetTypeParameters {
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        public String name();
        public String value();
    }

    public static void main(String[] args) {
        int result = 2;
        try {
            result = ClassGetTypeParameters1();
        } catch (Exception e) {
            e.printStackTrace();
            result = 3;
        }
        System.out.println(result);
    }

    public static int ClassGetTypeParameters1() {
        Class<MyTargetTest5> m;
        try {
            m = MyTargetTest5.class;
            TypeVariable<Class<MyTargetTest5>>[] target = m.getTypeParameters();
            if (target.length == 0) {
                return 0;
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        return 2;
    }

    class MyTargetTest5 {
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
        public MyTargetTest5(String home) {
            this.home = home;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
