/*
 *- @TestCaseID: ReflectionGetDeclaredField1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredField1.java
 *- @Title/Destination: Class.GetDeclaredField() Returns a Field object that reflects the specified declared field of
 *                      the class or interface represented by this Class object.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Create a test class.
 * -#step2: Use classloader to load class.
 * -#step3: Check that return field objects are correctly.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredField1.java
 *- @ExecuteClass: ReflectionGetDeclaredField1
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

class GetDeclaredField1 {
    public int i = 1;
    String s = "aaa";
    private double d = 2.5;
    protected float f = -222;
}

public class ReflectionGetDeclaredField1 {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success */
        try {
            Class zqp = Class.forName("GetDeclaredField1");
            Field zhu1 = zqp.getDeclaredField("i");
            Field zhu2 = zqp.getDeclaredField("s");
            Field zhu3 = zqp.getDeclaredField("d");
            Field zhu4 = zqp.getDeclaredField("f");
            if (zhu1.getName().equals("i") && zhu2.getName().equals("s") && zhu3.getName().equals("d") &&
                    zhu4.getName().equals("f")) {
                result = 0;
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            result = -1;
        } catch (NullPointerException e3) {
            System.err.println(e3);
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
