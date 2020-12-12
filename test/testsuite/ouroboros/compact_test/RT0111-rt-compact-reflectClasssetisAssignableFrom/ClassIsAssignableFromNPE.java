/*
 *- @TestCaseID: ClassIsAssignableFromNPE
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ClassIsAssignableFromNPE.java
 *- @Title/Destination: Test isAssignableFrom( null ) throws NullPointerException of class.
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 定义一个int数组intArray，获取intArray的class。
 *  -#step2：调用isAssignableFrom(Class<?> cls), cls为null。
 *  -#step3：确认抛出NullPointerException。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ClassIsAssignableFromNPE.java
 *- @ExecuteClass: ClassIsAssignableFromNPE
 *- @ExecuteArgs:
 */

public class ClassIsAssignableFromNPE {
    public static void main(String argv[]) {
        int result = 2; /* STATUS_FAILED */
        try {
            result = ClassIsAssignableFromNPE_1();
        } catch (Exception e) {
            System.out.println(e);
            result = 3;
        }
        System.out.println(result);
    }

    public static int ClassIsAssignableFromNPE_1() {
        int result1 = 4; /* STATUS_FAILED */
        int intArray[] = {1, 2, 3, 4, 5};
        Class cl = null;
        try {
            intArray.getClass().isAssignableFrom(cl);
        } catch (SecurityException e) {
            return 1;
        } catch (NullPointerException e) {
            return 0;
        }
        return result1;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
