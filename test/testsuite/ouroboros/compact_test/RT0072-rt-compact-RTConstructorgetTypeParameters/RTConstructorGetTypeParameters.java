/*
 *- @TestCaseID: RTConstructorGetTypeParameters
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTConstructorGetTypeParameters.java
 *- @Title/Destination: Constructor.getTypeParameters() Returns an array of TypeVariable objects that represent the type
 *                      variables declared by the generic declaration represented by this GenericDeclaration object, in
 *                      declaration order.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义一个类，含有类型变量的有参构造方法和无参的构造方法。
 * -#step2：调用Class.forName获取相应的class.
 * -#step3：使用class调用getConstructor(Class...<?> parameterTypes)从类中获取含有类型变量的构造方法1。
 * -#step4：调用getTypeParameters()获取TypeVariable数组，确认数组长度为5。
 * -#step5：使用class调用getDeclaredConstructor()从类中获取无参的构造方法。
 * -#step6：调用getTypeParameters()获取TypeVariable数组，确认数组长度为0。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTConstructorGetTypeParameters.java
 *- @ExecuteClass: RTConstructorGetTypeParameters
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;
import java.lang.reflect.TypeVariable;

class ConstructorGetTypeParameters {
    public <t, s, d, f, g> ConstructorGetTypeParameters(int number) {
    }

    ConstructorGetTypeParameters() {
    }
}

public class RTConstructorGetTypeParameters {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetTypeParameters");
            Constructor instance1 = cls.getConstructor(int.class);
            Constructor instance2 = cls.getDeclaredConstructor();
            TypeVariable[] q1 = instance1.getTypeParameters();
            TypeVariable[] q2 = instance2.getTypeParameters();
            if (q1.length == 5 && q2.length == 0) {
                System.out.println(0);
                return;
            }
            System.out.println(1);
            return;
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(3);
            return;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
