/*
 *- @TestCaseID: ReflectionCast1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionCast1.java
 *- @Title/Destination: Casts an object to the class or interface represented by this Class object
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 创建一个Cast1_a类的实例对象cast1_a，创建一个Cast1类的实例对象cast1；
 * -#step2: 将step1中的Cast1类的实例对象cast1强制转换为Cast1_a类型；
 * -#step3: 经转换后的cast1对象和Cast1_a类是同一类型；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionCast1.java
 *- @ExecuteClass: ReflectionCast1
 *- @ExecuteArgs:
 */

class Cast1 {
}

class Cast1_a extends Cast1 {
}

public class ReflectionCast1 {
    public static void main(String[] args) {
        Cast1_a cast1_a = new Cast1_a();
        Cast1 cast1 = new Cast1();
        cast1 = Cast1.class.cast(cast1_a);
        if (cast1.getClass().toString().equals("class Cast1_a")) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
