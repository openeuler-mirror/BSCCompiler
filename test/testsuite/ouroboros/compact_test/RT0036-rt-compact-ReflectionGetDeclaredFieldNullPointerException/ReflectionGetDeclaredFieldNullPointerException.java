/*
 *- @TestCaseID: ReflectionGetDeclaredFieldNullPointerException.
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredFieldNullPointerException.java
 *- @Title/Destination: Class.GetDeclaredField(null) throws NullPointerException.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Create two test class.
 * -#step2: Use classloader to load class.
 * -#step3: Test Class.GetDeclaredField() with null parameter.
 * -#step4: Check that NullPointerException was threw.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredFieldNullPointerException.java
 *- @ExecuteClass: ReflectionGetDeclaredFieldNullPointerException
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

class GetDeclaredField2_a {
    public int i_a = 5;
    String s_a = "bbb";
}

class GetDeclaredField2 extends GetDeclaredField2_a {
    public int i = 1;
    String s = "aaa";
    private double d = 2.5;
    protected float f = -222;
}

public class ReflectionGetDeclaredFieldNullPointerException {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        try {
            Class zqp = Class.forName("GetDeclaredField2");
            Field zhu1 = zqp.getDeclaredField("i_a");
            result = -1;
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        } catch (NullPointerException e2) {
            System.err.println(e2);
            result = -1;
        } catch (NoSuchFieldException e3) {
            try {
                Class zqp = Class.forName("GetDeclaredField2");
                Field zhu1 = zqp.getDeclaredField(null);
                result = -1;
            } catch (ClassNotFoundException e4) {
                System.err.println(e4);
                result = -1;
            } catch (NoSuchFieldException e5) {
                System.err.println(e5);
                result = -1;
            } catch (NullPointerException e6) {
                result = 0;
            }
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
