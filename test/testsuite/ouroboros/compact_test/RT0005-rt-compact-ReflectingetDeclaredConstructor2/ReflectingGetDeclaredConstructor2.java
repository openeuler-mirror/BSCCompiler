/*
 *- @TestCaseID: ReflectingGetDeclaredConstructor2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectingGetDeclaredConstructor2.java
 *- @Title/Destination: When the specified local constructor for the class can not be retrieved by reflection using
 *                      Class.getDeclaredConstructors(), NoSuchMethodException will be thrown
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过反射的方法获得GetDeclaredConstructor2类的一个实例对象getDeclaredConstructor2；
 * -#step2: 尝试通过step1中的getDeclaredConstructor2对象调用GetDeclaredConstructor2类的不存在的本地构造函数；
 * -#step3: step2中获取getDeclaredConstructor2对象的不存在的本地构造函数不成功，会抛出NoSuchMethodException；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectingGetDeclaredConstructor2.java
 *- @ExecuteClass: ReflectingGetDeclaredConstructor2
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;

class GetDeclaredConstructor2 {
    public GetDeclaredConstructor2() {
    }

    private GetDeclaredConstructor2(String name) {
    }

    protected GetDeclaredConstructor2(String name, int number) {
    }

    GetDeclaredConstructor2(int number) {
    }
}

public class ReflectingGetDeclaredConstructor2 {
    public static void main(String[] args) {
        try {
            Class getDeclaredConstructor2 = Class.forName("GetDeclaredConstructor2");
            Constructor constructor = getDeclaredConstructor2.getDeclaredConstructor(String.class, char.class, int.class);
            System.out.println(2);
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
