/*
 *- @TestCaseID: RTMethodGetTypeParameters
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTMethodGetTypeParameters.java
 *- @Title/Destination: Method.GetTypeParameters() returns an array of TypeVariable objects that represent the type
 *                      variables declared by the generic declaration in declaration order.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取MethodGetTypeParameters2类的运行时类clazz；
 * -#step2: 以ss、int.class为参数，通过getMethod()方法获取clazz的方法对象并记为method1；
 * -#step3: 以kk为参数，通过getDeclaredMethod()方法获取clazz的声明方法并记为method2；
 * -#step4: 通过getTypeParameters()方法分别获取method1、method2的所有的类型参数并记为typeVariables1、typeVariables2；
 * -#step5: 经判断得知typeVariables1的长度为5，typeVariables2的长度为0；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTMethodGetTypeParameters.java
 *- @ExecuteClass: RTMethodGetTypeParameters
 *- @ExecuteArgs:
 */

import java.lang.reflect.Method;
import java.lang.reflect.TypeVariable;

class MethodGetTypeParameters2 {
    public <t, s, d, f, g> void ss(int number) {
    }

    void kk() {
    }
}

public class RTMethodGetTypeParameters {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetTypeParameters2");
            Method method1 = clazz.getMethod("ss", int.class);
            Method method2 = clazz.getDeclaredMethod("kk");
            TypeVariable[] typeVariables1 = method1.getTypeParameters();
            TypeVariable[] typeVariables2 = method2.getTypeParameters();
            if (typeVariables1.length == 5 && typeVariables2.length == 0) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
