/*
 *- @TestCaseID: ReflectionCast3
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionCast3.java
 *- @Title/Destination: An exception is reported when a relationship-free object cannot be cast to a class represented
 *                      by this class object
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 创建一个Cast3_a类的实例对象cast3_a，创建一个Cast3类的实例对象cast3；
 * -#step2: 将Cast3_a类的对象cast3_a强制转换为Cast3类型；
 * -#step3: step2中的转换不能转换成功，会发生ClassCastException；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionCast3.java
 *- @ExecuteClass: ReflectionCast3
 *- @ExecuteArgs:
 */

class Cast3 {
}

class Cast3_a {
}

public class ReflectionCast3 {
    public static void main(String[] args) {
        Cast3_a cast3_a = new Cast3_a();
        Cast3 cast3 = new Cast3();
        try {
            cast3_a = Cast3_a.class.cast(cast3);
        } catch (ClassCastException e) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
