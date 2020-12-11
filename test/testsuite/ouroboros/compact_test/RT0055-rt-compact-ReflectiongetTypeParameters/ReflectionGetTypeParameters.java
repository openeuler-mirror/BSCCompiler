/*
 *- @TestCaseID: ReflectionGetTypeParameters
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetTypeParameters.java
 *- @Title/Destination: getTypeParameters() returns an array of TypeVariable objects that represent the type variables
 *                      declared by the generic declaration represented by this GenericDeclaration object, in
 *                      declaration order.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取GetTypeParameters类的运行时类clazz；
 * -#step2: 通过getTypeParameters()方法获取clazz的可变类型参数的数组并记为typeParameters；
 * -#step3: 确定step2中成功获取到typeParameters，并且typeParameters的长度为2，其第一个参数与“s”相同，第二个参数与“T”相同；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetTypeParameters.java
 *- @ExecuteClass: ReflectionGetTypeParameters
 *- @ExecuteArgs:
 */

import java.lang.reflect.TypeVariable;

class GetTypeParameters<s, T> {
}

public class ReflectionGetTypeParameters {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetTypeParameters");
            TypeVariable[] typeParameters = clazz.getTypeParameters();
            if (typeParameters.length == 2 && typeParameters[0].getName().equals("s")
                    && typeParameters[1].getName().equals("T")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
