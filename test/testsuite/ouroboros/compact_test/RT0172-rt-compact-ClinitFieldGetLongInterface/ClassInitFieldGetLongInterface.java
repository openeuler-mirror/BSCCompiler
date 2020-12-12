/*
 *- @TestCaseID: ClassInitFieldGetLongInterface
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldGetLongInterface.java
 *- @Title/Destination: When f is a field of interface OneInterface and call f.getLong(), OneInterface is initialized,
 *                      it's parent interface is not initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("OneInterface", false, OneInterface.class.getClassLoader()) and clazz.getField to get field f
 *          of OneInterface.
 * -#step2: Call method f.getLong(null), OneInterface is initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldGetLongInterface.java
 *- @ExecuteClass: ClassInitFieldGetLongInterface
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

public class ClassInitFieldGetLongInterface {
    static StringBuffer result = new StringBuffer("");

    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiLong");
            if (result.toString().compareTo("") == 0) {
                f.getLong(null);
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
    String aSuper = ClassInitFieldGetLongInterface.result.append("Super").toString();
}

@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldGetLongInterface.result.append("One").toString();
    long hiLong = 4859l;
}

interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldGetLongInterface.result.append("Two").toString();
}

@interface A {
    String aA = ClassInitFieldGetLongInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
