/*
 *- @TestCaseID: ReflectionAsSubclass1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionAsSubclass1.java
 *- @Title/Destination: child class calls Class.asSubclass() on father class/interface. Get the instance of the subclass
 *                      that casts the class to the target class by reflection gets the target
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过反射获取A_1类的运行时类，并对其返回值以A.class为参数调用asSubclass()方法，获得a_2；同理，通过反射获取
 *          A_2类的运行时类，并对其返回值以A.class为参数调用asSubclass()方法，获得a_1；
 * -#step2: 经判断得知a_2.newInstance()是A的一个实例，a_1.newInstance()也是A的一个实例；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionAsSubclass1.java
 *- @ExecuteClass: ReflectionAsSubclass1
 *- @ExecuteArgs:
 */

interface A {
}

class A1 implements A {
}

class A2 extends A1 {
}

public class ReflectionAsSubclass1 {
    public static void main(String[] args) {
        try {
            Class a2 = Class.forName("A1").asSubclass(A.class);
            Class a1 = Class.forName("A2").asSubclass(A.class);
            if (a2.newInstance() instanceof A) {
                if (a1.newInstance() instanceof A) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(e1);
        } catch (InstantiationException e2) {
            System.out.println(e2);
        } catch (IllegalAccessException e3) {
            System.out.println(e3);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
