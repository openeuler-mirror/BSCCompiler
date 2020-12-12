/*
 *- @TestCaseID: RTFieldGetName
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTFieldGetName.java
 *- @Title/Destination: Field.GetName() Returns the name of the field represented by this Field object.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义类FieldGetName。
 * -#step2: 通过调用forName()方法加载类FieldGetModifiers。
 * -#step3: 通过调用getField()、getDeclaredField()获取对应名称的成员变量。
 * -#step4: 通过调用getName()方法获取对应字段，调用equals()方法进行判断字段值正确。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTFieldGetName.java
 *- @ExecuteClass: RTFieldGetName
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

class FieldGetName {
    public static int num;
    final String str = "aaa";
}

public class RTFieldGetName {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetName");
            Field instance1 = cls.getField("num");
            Field instance2 = cls.getDeclaredField("str");
            if (instance1.getName().equals("num") && instance2.getName().equals("str")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
