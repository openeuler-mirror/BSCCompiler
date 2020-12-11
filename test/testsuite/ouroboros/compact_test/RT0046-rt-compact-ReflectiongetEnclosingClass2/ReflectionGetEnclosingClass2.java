/*
 *- @TestCaseID: ReflectionGetEnclosingClass2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetEnclosingClass2.java
 *- @Title/Destination: Returns null if the underlying class is a top-level class.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取GetEnclosingClassTest2类的运行时类clazz1；
 * -#step2: 通过(new GetEnclosingClassTest2()).test2().getClass()获取GetEnclosingClassTest2类的内部类classB类并记
 *          为clazz2；
 * -#step3: 通过clazz1.getEnclosingClass()的返回值为null，而通过clazz2.getEnclosingClass().getName()的返回值的类型
 *          与GetEnclosingClassTest2类相同；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetEnclosingClass2.java
 *- @ExecuteClass: ReflectionGetEnclosingClass2
 *- @ExecuteArgs:
 */

class GetEnclosingClassTest2 {
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

public class ReflectionGetEnclosingClass2 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetEnclosingClassTest2");
            Class clazz2 = (new GetEnclosingClassTest2()).test2().getClass();
            if (clazz1.getEnclosingClass() == null
                    && clazz2.getEnclosingClass().getName().equals("GetEnclosingClassTest2")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
