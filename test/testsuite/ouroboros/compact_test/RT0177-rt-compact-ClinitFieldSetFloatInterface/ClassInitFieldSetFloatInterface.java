/*
 *- @TestCaseID: ClassInitFieldSetFloatInterface
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldSetFloatInterface.java
 *- @Title/Destination: When f is a field of interface OneInterface and call f.setFloat(), OneInterface is initialized,
 *                      it's parent interface is not initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("OneInterface", false, OneInterface.class.getClassLoader()) and clazz.getField to get field f
 *          of OneInterface.
 * -#step2: Call method f.setFloat(null, 0.654f), OneInterface is initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldSetFloatInterface.java
 *- @ExecuteClass: ClassInitFieldSetFloatInterface
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

public class ClassInitFieldSetFloatInterface {
    static StringBuffer result = new StringBuffer("");

    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiFloat");
            if (result.toString().compareTo("") == 0) {
                f.setFloat(null, 0.654f);
            }
        }catch (IllegalAccessException e){
            result.append("IllegalAccessException");
        }catch (Exception e) {
            System.out.println(e);
        }

        if (result.toString().compareTo("OneIllegalAccessException") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}

interface SuperInterface{
    String aSuper = ClassInitFieldSetFloatInterface.result.append("Super").toString();
}

@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldSetFloatInterface.result.append("One").toString();
    float hiFloat = 0.25f;
}

interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldSetFloatInterface.result.append("Two").toString();
}

@interface A {
    String aA = ClassInitFieldSetFloatInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
