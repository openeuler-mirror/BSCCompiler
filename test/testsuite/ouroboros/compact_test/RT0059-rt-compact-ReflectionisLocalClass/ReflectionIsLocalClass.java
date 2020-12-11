/*
 *- @TestCaseID: ReflectionIsLocalClass
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionIsLocalClass.java
 *- @Title/Destination: isLocalClass() returns true if and only if the underlying class is a local class(defined in the
 *                      body of a method)
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Define a class in method and two class out of the method.
 * -#step2: Get Class object of the class.
 * -#step3: Test isLocalClass() with different the Class object of the class.
 * -#step4: Check that isLocalClass() identifies local class correctly.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionIsLocalClass.java
 *- @ExecuteClass: ReflectionIsLocalClass
 *- @ExecuteArgs:
 */

public class ReflectionIsLocalClass {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        class isLocalClass {
        }
        try {
            Class zqp1 = isLocalClass.class;
            Class zqp2 = Class.forName("ReflectionIsLocalClass");
            Class zqp3 = IsLocalClass_a.class;
            Class zqp4 = IsLocalClass_b.class;
            Class zqp5 = (new isLocalClass() {
            }).getClass();
            if (!zqp2.isLocalClass()) {
                if (!zqp3.isLocalClass()) {
                    if (!zqp4.isLocalClass()) {
                        if (!zqp5.isLocalClass()) {
                            if (zqp1.isLocalClass()) {
                                result = 0;
                            }
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            result = 2;
        }
        System.out.println(result);
    }

    class IsLocalClass_a {
    }

    static class IsLocalClass_b {
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
