/*
 *- @TestCaseID: RTMethodIsDefault
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTMethodIsDefault.java
 *- @Title/Destination: Verify through reflection whether the default method in the interface.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法分别获取MethodIsDefault_A类、MethodIsDefault类的运行时类clazz1、clazz2；
 * -#step2: 以run为参数，通过getMethod()方法分别获取clazz1、clazz2的方法对象并记为method1、method2；
 * -#step3: 以ss、int.class为参数，通过getDeclaredMethod()方法获取clazz2的声明方法并记为method3；
 * -#step4: 经isDefault()判断得知method1是默认的；
 * -#step5: 经isDefault()判断得知method2、method3不是默认的；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTMethodIsDefault.java
 *- @ExecuteClass: RTMethodIsDefault
 *- @ExecuteArgs:
 */

import java.lang.reflect.Method;

interface MethodIsDefault_A {
    default public void run() {
        String i;
    }
}

class MethodIsDefault implements MethodIsDefault_A {
    public void run() {
        String i = "abc";
    }

    void ss(int number) {
    }
}

public class RTMethodIsDefault {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("MethodIsDefault_A");
            Class clazz2 = Class.forName("MethodIsDefault");
            Method method1 = clazz1.getMethod("run");
            Method method2 = clazz2.getMethod("run");
            Method method3 = clazz2.getDeclaredMethod("ss", int.class);
            if (method1.isDefault()) {
                if (!method2.isDefault() && !method3.isDefault()) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e) {
            System.err.println(e);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
