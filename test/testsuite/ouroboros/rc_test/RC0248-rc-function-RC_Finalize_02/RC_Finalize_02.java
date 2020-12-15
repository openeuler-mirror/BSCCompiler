/*
 *- @TestCaseID:maple/runtime/rc/function/RC_Finalize_02.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test Finalizer for RC .
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RC_Finalize_02.java
 *- @ExecuteClass: RC_Finalize_02
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.io.PrintStream;

public class RC_Finalize_02 {

    public void finalize() { 
	System.out.println("ExpectResult");
    }

    public static int run(String argv[], PrintStream out) {
	return 0;
    }

    public static void main(String argv[]) {

	System.runFinalizersOnExit(true);
	RC_Finalize_02 testClass = new RC_Finalize_02();
    }
} // end RC_Finalize_02
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
