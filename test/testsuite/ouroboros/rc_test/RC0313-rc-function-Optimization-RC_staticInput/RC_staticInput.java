/*
 *- @TestCaseID:maple/runtime/rc/function/RC_staticInput.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\nExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RC_staticInput.java
 *- @ExecuteClass: RC_staticInput
 *- @ExecuteArgs:
 *- @Remark:
 */
public class RC_staticInput {
    static int []test= {10,20,30,40};

    public static void main(String [] args) {
        if(test.length==4)
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
        test(4,test);
        if(test.length==4)
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
    }
    public static void test(int first, int[] second) {
        int [] xyz = {23,24,25,26};
        test = xyz;
        if(second.length==4)
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\nExpectResult\n
