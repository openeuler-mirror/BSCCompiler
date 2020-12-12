/*
 *- @TestCaseID: ClassInitFieldGetCharInterface
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldGetCharInterface.java
 *- @Title/Destination: When f is a field of interface OneInterface and call f.getChar(), OneInterface is initialized,
 *                      it's parent interface is not initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("OneInterface", false, OneInterface.class.getClassLoader()) and clazz.getField to get field f
 *          of OneInterface.
 * -#step2: Call method f.getChar(null), OneInterface is initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldGetCharInterface.java
 *- @ExecuteClass: ClassInitFieldGetCharInterface
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

public class ClassInitFieldGetCharInterface {
    static StringBuffer result = new StringBuffer("");

    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiChar");
            if (result.toString().compareTo("") == 0) {
                f.getChar(null);
            }
        } catch (Exception e) {
            System.out.println(e);
        }

        if (result.toString().compareTo("One") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}

interface SuperInterface{
    String aSuper = ClassInitFieldGetCharInterface.result.append("Super").toString();
}

@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldGetCharInterface.result.append("One").toString();
    char hiChar = (char)45;
}

interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldGetCharInterface.result.append("Two").toString();
}

@interface A {
    String aA = ClassInitFieldGetCharInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
