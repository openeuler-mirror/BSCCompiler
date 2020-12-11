/*
 *- @TestCaseID: ReflectingGetConstructor2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectingGetConstructor2.java
 *- @Title/Destination: When the specified constructor could not be found by reflection, NoSuchMethodException is thrown
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获得GetConstructor2类的一个实例对象getConstructor21；
 * -#step2: 以int.class为参数，尝试通过getConstructor()方法获取GetConstructor2类的构造函数并记为constructor1；
 * -#step3: 通过Class.forName()方法获得GetConstructor2类的一个实例对象getConstructor22；
 * -#step4: 以String.class, char.class, int.class为参数，尝试通过getConstructor()方法获取GetConstructor2类的构造函数并记
 *          为constructor2；
 * -#step5: 确定step2和step4中的构造函数constructor1、constructor2获取失败，并且会抛出NoSuchMethodException；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectingGetConstructor2.java
 *- @ExecuteClass: ReflectingGetConstructor2
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;

class GetConstructor2 {
    public GetConstructor2() {
    }

    public GetConstructor2(String name) {
    }

    public GetConstructor2(String name, int number) {
    }

    GetConstructor2(int number) {
    }
}

public class ReflectingGetConstructor2 {
    public static void main(String[] args) {
        try {
            Class getConstructor21 = Class.forName("GetConstructor2");
            Constructor constructor1 = getConstructor21.getConstructor(int.class);
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e) {
            try {
                Class getConstructor22 = Class.forName("GetConstructor2");
                Constructor constructor2 = getConstructor22.getConstructor(String.class, char.class, int.class);
                System.out.println(2);
            } catch (ClassNotFoundException ee) {
                System.err.println(ee);
                System.out.println(2);
            } catch (NoSuchMethodException ee) {
                System.out.println(0);
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
