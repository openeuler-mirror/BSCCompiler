/*
 *- @TestCaseID: ReflectionGetEnclosingClass1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetEnclosingClass1.java
 *- @Title/Destination: Class.getEnclosingClass() Get the immediately enclosing class of the underlying class by
 *                      reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过(new GetEnclosingClassTest1()).test1().getClass()获取ReflectionGetEnclosingClass1类的内部类
 *          GetEnclosingClassTest1类并记为clazz1；
 * -#step2: 通过(new GetEnclosingClassTest1()).test2().getClass()获取ReflectionGetEnclosingClass1类的内部类
 *          GetEnclosingClassTest1类并记为clazz2；
 * -#step3: 确定step1和step2中成功获取到ReflectionGetEnclosingClass1类的两个内部类clazz1、clazz2；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetEnclosingClass1.java
 *- @ExecuteClass: ReflectionGetEnclosingClass1
 *- @ExecuteArgs:
 */

public class ReflectionGetEnclosingClass1 {
    public static void main(String[] args) {
        Class clazz1 = (new GetEnclosingClassTest1()).test1().getClass();
        Class clazz2 = (new GetEnclosingClassTest1()).test2().getClass();
        if (clazz1.getEnclosingClass().getName().equals("ReflectionGetEnclosingClass1$GetEnclosingClassTest1")
                && clazz2.getEnclosingClass().getName().equals("ReflectionGetEnclosingClass1$GetEnclosingClassTest1")) {
            System.out.println(0);
        }
    }

    public static class GetEnclosingClassTest1 {
        public Object test1() {
            class classA {
            }
            return new classA();
        }

        Object test2() {
            class classB {
            }
            return new classB();
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
