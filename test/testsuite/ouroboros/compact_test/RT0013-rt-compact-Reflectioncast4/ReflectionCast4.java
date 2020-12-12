/*
 *- @TestCaseID: ReflectionCast4
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionCast4.java
 *- @Title/Destination: Object cast to NULL
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 创建一个Cast4类的实例对象cast4，创建一个Object类的变量object，并赋初值为null；
 * -#step2: 将Cast4类的实例对象cast4强制转换为null；
 * -#step3: step2中成功将cast4对象转换为null；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionCast4.java
 *- @ExecuteClass: ReflectionCast4
 *- @ExecuteArgs:
 */

class Cast4 {
}

public class ReflectionCast4 {
    public static void main(String[] args) {
        Object object = null;
        Cast4 cast4 = new Cast4();
        cast4 = Cast4.class.cast(object);
        if (cast4 == null) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
