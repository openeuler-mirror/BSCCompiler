/*
 *- @TestCaseID: ReflectionGetComponentType
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetComponentType.java
 *- @Title/Destination: Class.getComponentType() returns the Class representing the component type of an array. If
 *                      this class does not represent an array class ,then this method returns null.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Test Class.getComponentType() when char,string[] and int[] call it.
 * -#step2: Check Class.getComponentType() returns right component type when an array calls it.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetComponentType.java
 *- @ExecuteClass: ReflectionGetComponentType
 *- @ExecuteArgs:
 */

public class ReflectionGetComponentType {

    public static void main(String[] args) {
        if (char.class.getComponentType() == null && String[].class.getComponentType().toString().
                equals("class java.lang.String") && int[].class.getComponentType().toString().equals("int")) {
            System.out.println(0);
        }else{
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
