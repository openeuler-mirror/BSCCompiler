/*
 *- @TestCaseID: ReflectingGetConstructor1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectingGetConstructor1.java
 *- @Title/Destination: Getting public constructors of different arguments from reflection by calling
 *                      Class.getConstructor()
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName获得GetConstructor1类的一个实例对象getConstructor1；
 * -#step2: 分别调用GetConstructor1类的三个构造方法，从而实现从反射中获取不同参数的公共构造函数constructor1、constructor2、
 *          constructor3；
 * -#step3: 判断三个构造方法获取成功；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectingGetConstructor1.java
 *- @ExecuteClass: ReflectingGetConstructor1
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;

class GetConstructor1 {
    public GetConstructor1() {
    }

    public GetConstructor1(String name) {
    }

    public GetConstructor1(String name, int number) {
    }

    GetConstructor1(int number) {
    }
}

public class ReflectingGetConstructor1 {
    public static void main(String[] args) {
        try {
            Class getConstructor1 = Class.forName("GetConstructor1");
            Constructor constructor1 = getConstructor1.getConstructor(String.class);
            Constructor constructor2 = getConstructor1.getConstructor();
            Constructor constructor3 = getConstructor1.getConstructor(String.class, int.class);
            if (constructor1.toString().equals("public GetConstructor1(java.lang.String)")
                    && constructor2.toString().equals("public GetConstructor1()")
                    && constructor3.toString().equals("public GetConstructor1(java.lang.String,int)")) {
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
