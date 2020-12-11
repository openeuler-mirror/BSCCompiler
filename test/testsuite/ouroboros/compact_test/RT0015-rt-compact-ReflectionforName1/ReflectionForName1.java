/*
 *- @TestCaseID: ReflectionForName1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionForName1.java
 *- @Title/Destination: Use Class.forName() To find a class by class name
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName的方法获取ForName1类的类型clazz；
 * -#step2: 确定step1中的clazz与ForName1类是同一类型；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionForName1.java
 *- @ExecuteClass: ReflectionForName1
 *- @ExecuteArgs:
 */

class ForName1 {
}

public class ReflectionForName1 {
    public static void main(String[] args) throws ClassNotFoundException {
        Class clazz = Class.forName("ForName1");
        if (clazz.toString().equals("class ForName1")) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
