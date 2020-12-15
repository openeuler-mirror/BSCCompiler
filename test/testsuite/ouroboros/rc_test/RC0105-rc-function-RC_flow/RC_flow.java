/*
 *- @TestCaseID:maple/runtime/rc/function/RC_flow.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test RC basic operation in For flow.
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RC_flow.java
 *- @ExecuteClass: RC_flow
 *- @ExecuteArgs:
 *- @Remark:
 */
public class RC_flow {
    private Object obj;
    private byte[] b;

    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
        System.out.println("ExpectResult");
    }

    public static void rc_testcase_main_wrapper() {
        TestFlow1(21);
        TestFlow2(30);
        TestFlow3(56);
    }

    static Object TestFlow1(int num) {
        Object rc = null;
        for (int i = 1; i < num; i++) {
            if ((i % 5) == 0) {
                rc = new RC_flow();
            }
        }
        return rc;
    }

    static Object TestFlow2(int num) {
        Object rc = null;
        for (int i = 1; i < num; i++) {
            if ((i % 5) == 0) {
                rc = Test2();
            }
        }
        return rc;
    }

    static void TestFlow3(int num) {
        Object rc = null;
        for (int i = 1; i < num; i++) {
            if ((i % 5) == 0) {
                rc = Test2();
            }
        }

    }

    static RC_flow Test2() {
        RC_flow obj = new RC_flow();
        return obj;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
