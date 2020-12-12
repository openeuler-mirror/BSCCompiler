/*
 *- @TestCaseID: RTMethodToGenericString
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTMethodToGenericString.java
 *- @Title/Destination: Method.toGenericString() Returns a string describing this Method, including type parameters.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取MethodToGenericString类的运行时类clazz；
 * -#step2: 以ss、int.class为参数，通过getMethod()方法获取clazz的方法对象并记为method；
 * -#step3: 通过method.toGenericString()返回的表示method的方法名称和类型参数与字符串
 *          “public <t,s,d,f,g> void MethodToGenericString.ss(int)”相同；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTMethodToGenericString.java
 *- @ExecuteClass: RTMethodToGenericString
 *- @ExecuteArgs:
 */

import java.lang.reflect.Method;

class MethodToGenericString {
    public <t, s, d, f, g> void ss(int number) {
    }
}

public class RTMethodToGenericString {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodToGenericString");
            Method method = clazz.getMethod("ss", int.class);
            if (method.toGenericString().equals("public <t,s,d,f,g> void MethodToGenericString.ss(int)")) {
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
