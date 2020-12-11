/*
 *- @TestCaseID: ReflectionForName4
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionForName4.java
 *- @Title/Destination: An exception is reported when a class is not found with reflection through a class name
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 以“for*”、true、ForName4_a.class.getClassLoader()为参数，通过Class.forName()获取相关类的运行时类时会
 *          抛出ClassNotFoundException；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionForName4.java
 *- @ExecuteClass: ReflectionForName4
 *- @ExecuteArgs:
 */

class ForName4 {
    static {
    }
}

class ForName4_a {
    static {
    }
}

public class ReflectionForName4 extends Thread {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("for*", true, ForName4_a.class.getClassLoader());
        } catch (ClassNotFoundException e) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
