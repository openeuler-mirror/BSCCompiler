/*
 *- @TestCaseID: RTModifierParameterModifiers
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTModifierParameterModifiers.java
 *- @Title/Destination: Verify that the integer value of the parameter modifier for the modifier class is 16.
 *                      Modifier.parameterModifiers() return an int value OR-ing together the source language modifiers
 *                      that can be applied to a parameter.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 经判断得知Modifier.parameterModifiers()的返回值为16，即修饰符类的参数修饰符的整数值为16；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTModifierParameterModifiers.java
 *- @ExecuteClass: RTModifierParameterModifiers
 *- @ExecuteArgs:
 */

import java.lang.reflect.Modifier;

public class RTModifierParameterModifiers {
    public static void main(String[] args) {
        if (Modifier.parameterModifiers() == 16) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
