/*
 *- @TestCaseID: ReflectionGetDeclaredFields
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredFields.java
 *- @Title/Destination: Class.getDeclaredFields() returns an array of Field objects reflecting all the fields declared
 *                      by the class or interface represented by this Class object. This includes public, protected,
 *                      default (package) access, and private fields, but excludes inherited fields.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Create test classes and enum.
 * -#step2: Use classloader to load class.
 * -#step3: Check that return Field objects array  which exclude inherited fields which  by calling GetDeclaredFields().
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredFields.java
 *- @ExecuteClass: ReflectionGetDeclaredFields
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

class GetDeclaredFields_a {
    public int i_a = 5;
    String s_a = "bbb";
}

class GetDeclaredFields extends GetDeclaredFields_a {
}

enum GetDeclaredFields_b {
    i_b, s_b, f_b
}

public class ReflectionGetDeclaredFields {
    public static void main(String[] args) {
        int result = 0;
        try {
            Class zqp1 = Class.forName("GetDeclaredFields");
            Class zqp2 = Class.forName("GetDeclaredFields_b");
            Field[] j = zqp1.getDeclaredFields();
            Field[] k = zqp2.getDeclaredFields();
            if (j.length == 0 && k.length == 4) {
                result = 0;
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
