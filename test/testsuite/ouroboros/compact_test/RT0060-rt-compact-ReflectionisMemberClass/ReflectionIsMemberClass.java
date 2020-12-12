/*
 *- @TestCaseID: ReflectionIsMemberClass
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionIsMemberClass.java
 *- @Title/Destination: Class.IsMemberClass() returns true if and only if the underlying class is a member
 *                      class(class inside another class, but not inside a method).
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Define a class in method and two inner class but not inside the method.
 * -#step2: Get Class object of the class.
 * -#step3: Test isMemberClass() with different the Class object of the class.
 * -#step4: Check that isMemberClass() identifies member class correctly.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionIsMemberClass.java
 *- @ExecuteClass: ReflectionIsMemberClass
 *- @ExecuteArgs:
 */

public class ReflectionIsMemberClass {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        class IsMemberClass_a {
        }
        try {
            Class zqp1 = IsMemberClass.class;
            Class zqp2 = Class.forName("ReflectionIsMemberClass");
            Class zqp3 = IsMemberClass_a.class;
            Class zqp4 = IsMemberClass_b.class;
            Class zqp5 = (new IsMemberClass_b() {
            }).getClass();
            if (!zqp2.isMemberClass()) {
                if (!zqp3.isMemberClass()) {
                    if (zqp4.isMemberClass()) {
                        if (!zqp5.isMemberClass()) {
                            if (zqp1.isMemberClass()) {
                                result = 0;
                            }
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            result = -1;
        }
        System.out.println(result);
    }

    class IsMemberClass {
    }

    static class IsMemberClass_b {
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
