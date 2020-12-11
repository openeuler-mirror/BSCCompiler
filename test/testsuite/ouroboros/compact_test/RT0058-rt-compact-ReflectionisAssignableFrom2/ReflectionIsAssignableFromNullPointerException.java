/*
 *- @TestCaseID: ReflectionIsAssignableFromNullPointerException
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionIsAssignableFromNullPointerException.java
 *- @Title/Destination: Class.isAssignableFrom(null) throws NullPointerException.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Use classloader load class.
 * -#step2: Test Class.isAssignableFrom() whit null param.
 * -#step3: Check that NullPointerException occurs when Class.isAssignableFrom()'s is null.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionIsAssignableFromNullPointerException.java
 *- @ExecuteClass: ReflectionIsAssignableFromNullPointerException
 *- @ExecuteArgs:
 */

class AssignableFromNullPointerException {
}

public class ReflectionIsAssignableFromNullPointerException {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        try {
            Class zqp1 = Class.forName("AssignableFromNullPointerException");
            zqp1.isAssignableFrom(null);
            result = -1;
        } catch (ClassNotFoundException e1) {
            result = -1;
        } catch (NullPointerException e2) {
            result = 0;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
