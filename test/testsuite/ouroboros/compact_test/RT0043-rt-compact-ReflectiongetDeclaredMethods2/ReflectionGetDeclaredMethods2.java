/*
 *- @TestCaseID: ReflectionGetDeclaredMethods2
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredMethods2.java
 *- @Title/Destination: Class.getDeclaredMethods() does not return inherited methods.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取GetDeclaredMethods2类的运行时类clazz；
 * -#step2: 通过getDeclaredMethods()方法获取clazz的所有的方法对象，返回值为一个数组，并记为method；
 * -#step3: 确定method的长度为0，即返回的数组为空，证明通过getDeclaredMethods()方法不返回继承的方法。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredMethods2.java
 *- @ExecuteClass: ReflectionGetDeclaredMethods2
 *- @ExecuteArgs:
 */

import java.lang.reflect.Method;

class GetDeclaredMethods2_a {
    public void empty1() {
    }

    public void empty2() {
    }
}

class GetDeclaredMethods2 extends GetDeclaredMethods2_a {
}

public class ReflectionGetDeclaredMethods2 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetDeclaredMethods2");
            Method[] method = clazz.getDeclaredMethods();
            if (method.length == 0) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
