/*
 *- @TestCaseID: RTConstructorToGenericString
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTConstructorToGenericString1.java
 *- @Title/Destination: Constructor.ToGenericString() returns a string describing this Constructor, including type
 *                      parameters.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义一个类，含有类型变量的有参构造方法。
 * -#step2：调用Class.forName获取相应的class.
 * -#step3：使用class调用getConstructor(Class...<?> parameterTypes)从类中获取含有类型变量的构造方法。
 * -#step4：调用toGenericString()获取描述此Method的字符串。
 * -#step5：确认返回的描述正确。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTConstructorToGenericString1.java
 *- @ExecuteClass: RTConstructorToGenericString1
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;

class ConstructorToGenericString1 {
    public <t, s, d, f, g> ConstructorToGenericString1(int number) {
    }
}

public class RTConstructorToGenericString1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorToGenericString1");
            Constructor instance1 = cls.getConstructor(int.class);
            if (instance1.toGenericString().equals("public <t,s,d,f,g> ConstructorToGenericString1(int)")) {
                System.out.println(0);
                return;
            }
        } catch (ClassNotFoundException e1) {
            System.out.println(1);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(2);
            return;
        }
        System.out.println(3);
        return;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
