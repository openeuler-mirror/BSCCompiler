/*
 *- @TestCaseID: ReflectionAsSubclass2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionAsSubclass2.java
 *- @Title/Destination: To cast a subclass of another class by reflection get a class
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName("B_1")，再对其返回值调用asSubclass()方法可以获得B类的子类B_1类；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionAsSubclass2.java
 *- @ExecuteClass: ReflectionAsSubclass2
 *- @ExecuteArgs:
 */

class B {
}

class B_1 extends B {
}

public class ReflectionAsSubclass2 {
    public static void main(String[] args) {
        try {
            Class.forName("B_1").asSubclass(B.class);
        } catch (ClassCastException e) {
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
        System.out.println(0);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
