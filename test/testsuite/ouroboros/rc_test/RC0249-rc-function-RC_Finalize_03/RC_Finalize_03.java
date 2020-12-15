/*
 *- @TestCaseID:maple/runtime/rc/function/RC_Finalize_03.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test Finalizer for RC .
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RC_Finalize_03.java
 *- @ExecuteClass: RC_Finalize_03
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.io.PrintStream;
class RC_Finalize_031 {
    public void finalize() {
        RC_Finalize_03.fo_finalized = true;
        RC_Finalize_03.dummy = 1 / RC_Finalize_03.zero;
        RC_Finalize_03.fo_exception_occerred = false;
    }
}
public class RC_Finalize_03 {

    public static boolean fo_finalized;
    public static boolean fo_exception_occerred;
    public static final long TIMEOUT = 50000;
    public static final int zero = 0;
    public static int dummy;

    public static int run(String argv[], PrintStream out) {

        fo_finalized = false;
        fo_exception_occerred = true;

        RC_Finalize_031 cl1 = new RC_Finalize_031();
        cl1 = null;
        long startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < TIMEOUT){
            Runtime.getRuntime().gc();
            Runtime.getRuntime().runFinalization();
            if (!fo_finalized) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    //out.println("InterruptedException: " + e);
                    return 2;
                } catch(Throwable e) {
                    //out.println("Throwable: " + e);
                    return 2;
                }
            } else {
                break;
            }
        }
        if (!fo_finalized) {
            //out.println("Ok, RC_Finalize_031 was not finalized during " + TIMEOUT/1000 + "sec");
            return 0;
        }

        if (fo_exception_occerred) {
            return 0;
        } else {
            //out.println("Failed: expected exception is not thrown");
        }

        return 2;
    }

    public static void main(String argv[]) {
        if(run(argv, System.out)==0)
			System.out.println("ExpectResult");

    }
} // end RC_Finalize_03
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
