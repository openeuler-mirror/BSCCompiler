/*
 *- @TestCaseID: ReflectionToString
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionToString.java
 *- @Title/Destination: To obtain a class for string output by reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Create a test class.
 * -#step2: Use classloader to load class.
 * -#step3: Test toString() by reflection.
 * -#step4: Check that reflection result is correct.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionToString.java
 *- @ExecuteClass: ReflectionToString
 *- @ExecuteArgs:
 */

@interface Fff {
}

@Fff
abstract class ToString_$<t> {
}

public class ReflectionToString {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        try {
            Class zqp = Class.forName("ToString_$");
            String zhu = zqp.toString();
            if (zhu.equals("class ToString_$")) {
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
