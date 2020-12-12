/*
 *- @TestCaseID: ReflectionGetCanonicalName
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetCanonicalName.java
 *- @Title/Destination: Class.getCanonicalName() returns class name by reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Define an empty class.
 * -#step2: Get Class instance by calling ForName(String className)
 * -#step3: Test getCanonicalName() returns class name correctly.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetCanonicalName.java
 *- @ExecuteClass: ReflectionGetCanonicalName
 *- @ExecuteArgs:
 */

class GetCanonicalNameTest {
}

public class ReflectionGetCanonicalName {

    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */

        try {
            Class zqp = Class.forName("GetCanonicalNameTest");
            if (zqp.getCanonicalName().equals("GetCanonicalNameTest")) {
                result = 0;
            }
        } catch (ClassNotFoundException e) {
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
