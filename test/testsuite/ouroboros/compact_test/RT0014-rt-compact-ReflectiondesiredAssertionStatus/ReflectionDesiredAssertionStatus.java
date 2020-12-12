/*
 *- @TestCaseID: ReflectionDesiredAssertionStatus
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionDesiredAssertionStatus.java
 *- @Title/Destination: When a class's assertionStatus is not set, desiredAssertionStatus() return false
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 创建一个ReflectionDesiredAssertionStatus类的实例对象reflectionDesiredAssertionStatus；
 * -#step2: 通过reflectionDesiredAssertionStatus的getClass()方法获取其所属的类型并记为clazz；
 * -#step3: 调用clazz的desiredAssertionStatus()方法得到的返回值为false；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionDesiredAssertionStatus.java
 *- @ExecuteClass: ReflectionDesiredAssertionStatus
 *- @ExecuteArgs:
 */

public class ReflectionDesiredAssertionStatus {
    public static void main(String[] args) {
        ReflectionDesiredAssertionStatus reflectionDesiredAssertionStatus = new ReflectionDesiredAssertionStatus();
        Class clazz = reflectionDesiredAssertionStatus.getClass();
        if (!clazz.desiredAssertionStatus()) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
