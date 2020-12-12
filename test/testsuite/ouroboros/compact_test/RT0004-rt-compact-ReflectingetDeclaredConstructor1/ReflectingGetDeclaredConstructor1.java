/*
 *- @TestCaseID: ReflectingGetDeclaredConstructor1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectingGetDeclaredConstructor1.java
 *- @Title/Destination: Call Class.getDeclaredConstructor() to get the local constructor of a class by reflection
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过反射的方法获得GetDeclaredConstructor1类的一个实例对象getDeclaredConstructor1；
 * -#step2: 通过step1中获得的getDeclaredConstructor1分别调用GetDeclaredConstructor1类的三个构造方法，从而实现从反射中获
 *          取不同参数的本地构造函数constructor1、constructor2、constructor3；
 * -#step3: step2中所获取到的三个公共构造函数constructor1、constructor2、constructor3分别对应于
 *          GetDeclaredConstructor1类的三个构造函数，表明本地构造函数获取成功；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectingGetDeclaredConstructor1.java
 *- @ExecuteClass: ReflectingGetDeclaredConstructor1
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;

class GetDeclaredConstructor1 {
    public GetDeclaredConstructor1() {
    }

    private GetDeclaredConstructor1(String name) {
    }

    protected GetDeclaredConstructor1(String name, int number) {
    }

    GetDeclaredConstructor1(int number) {
    }
}

public class ReflectingGetDeclaredConstructor1 {
    public static void main(String[] args) {
        try {
            Class getDeclaredConstructor1 = Class.forName("GetDeclaredConstructor1");
            Constructor constructor1 = getDeclaredConstructor1.getDeclaredConstructor(String.class);
            Constructor constructor2 = getDeclaredConstructor1.getDeclaredConstructor();
            Constructor constructor3 = getDeclaredConstructor1.getDeclaredConstructor(String.class, int.class);
            Constructor constructor4 = getDeclaredConstructor1.getDeclaredConstructor(int.class);
            if (constructor1.toString().equals("private GetDeclaredConstructor1(java.lang.String)")
                    && constructor2.toString().equals("public GetDeclaredConstructor1()")
                    && constructor3.toString().equals("protected GetDeclaredConstructor1(java.lang.String,int)")
                    && constructor4.toString().equals("GetDeclaredConstructor1(int)")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
