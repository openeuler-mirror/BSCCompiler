/*
 *- @TestCaseID: ClassInitFieldGetDoubleInterface
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldGetDoubleInterface.java
 *- @Title/Destination: When f is a field of interface OneInterface and call f.getDouble(), OneInterface is initialized,
 *                      it's parent interface is not initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("OneInterface", false, OneInterface.class.getClassLoader()) and clazz.getField to get field f
 *          of OneInterface.
 * -#step2: Call method f.getDouble(null), OneInterface is initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldGetDoubleInterface.java
 *- @ExecuteClass: ClassInitFieldGetDoubleInterface
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

public class ClassInitFieldGetDoubleInterface {
    static StringBuffer result = new StringBuffer("");

    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiDouble");
            if (result.toString().compareTo("") == 0) {
                f.getDouble(null);
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
    String aSuper = ClassInitFieldGetDoubleInterface.result.append("Super").toString();
}

@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldGetDoubleInterface.result.append("One").toString();
    double hiDouble = 0.1532;
}

interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldGetDoubleInterface.result.append("Two").toString();
}

@interface A {
    String aA = ClassInitFieldGetDoubleInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
