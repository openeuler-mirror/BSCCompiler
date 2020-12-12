/*
 *- @TestCaseID: ClassInitFieldOtherMethodInterface
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassInitFieldOtherMethodInterface.java
 *- @Title/Destination: When f is a field of of interface OneInterface and call method except setXX/getXX, class is not
 *                      initialized.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: Class.forName("OneInterface", false, OneInterface.class.getClassLoader()) and clazz.getField to get field f
 *          of OneInterface.
 * -#step2: Call method of Field except setXX/getXX, class One is not initialized.
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassInitFieldOtherMethodInterface.java
 *- @ExecuteClass: ClassInitFieldOtherMethodInterface
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

public class ClassInitFieldOtherMethodInterface {
    static StringBuffer result = new StringBuffer("");

    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hi");

            f.equals(f);
            f.getAnnotation(A.class);
            f.getAnnotationsByType(A.class);
            f.getDeclaredAnnotations();
            f.getDeclaringClass();
            f.getGenericType();
            f.getModifiers();
            f.getName();
            f.getType();
            f.hashCode();
            f.isEnumConstant();
            f.isSynthetic();
            f.toGenericString();
            f.toString();
        } catch (Exception e) {
            System.out.println(e);
        }

        if(result.toString().compareTo("") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}

interface SuperInterface{
    String aSuper = ClassInitFieldOtherMethodInterface.result.append("Super").toString();
}

interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldOtherMethodInterface.result.append("One").toString();
    @A
    short hi = 14;
}

interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldOtherMethodInterface.result.append("Two").toString();
}

@interface A {
    String aA = ClassInitFieldOtherMethodInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
