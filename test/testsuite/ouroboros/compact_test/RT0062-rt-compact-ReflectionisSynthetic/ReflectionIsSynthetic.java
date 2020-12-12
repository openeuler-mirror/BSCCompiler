/*
 *- @TestCaseID: ReflectionIsSynthetic
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionIsSynthetic.java
 *- @Title/Destination: Returns true if this class is a synthetic class
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Define a test class IsSynthetic.
 * -#step2: Test isSynthetic() with three different type object.
 * -#step3: Check that isSynthetic() identifies synthetic class correctly.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionIsSynthetic.java
 *- @ExecuteClass: ReflectionIsSynthetic
 *- @ExecuteArgs:
 */

public class ReflectionIsSynthetic {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        new IsSynthetic();
        try {
            Class zqp1 = IsSynthetic.class;
            Class zqp2 = int.class;
            Class zqp3 = Class.forName("ReflectionIsSynthetic$1");
            if (!zqp2.isSynthetic()) {
                if (!zqp1.isSynthetic()) {
                    if (zqp3.isSynthetic()) {
                        result = 0;
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            result = -1;
        }
        System.out.println(result);
    }

    private static class IsSynthetic {
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
