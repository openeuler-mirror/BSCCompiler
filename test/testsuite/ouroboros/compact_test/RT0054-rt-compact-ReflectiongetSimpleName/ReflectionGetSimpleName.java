/*
 *- @TestCaseID: ReflectionGetSimpleName
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetSimpleName.java
 *- @Title/Destination: Returns the simple name of the underlying class as given in the source code.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取GetSimpleName类的运行时类clazz；
 * -#step2: 通过getSimpleName()方法获取clazz的类名并记为string；
 * -#step3: 确定step2中成功获取到string并且其类型与GetSimpleName类是同一类型；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetSimpleName.java
 *- @ExecuteClass: ReflectionGetSimpleName
 *- @ExecuteArgs:
 */

class GetSimpleName {
}

public class ReflectionGetSimpleName {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetSimpleName");
            String string = clazz.getSimpleName();
            if (string.equals("GetSimpleName")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
