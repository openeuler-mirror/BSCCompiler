/*
 * -@TestCaseID: T6481655
 * -@TestCaseName: T6481655
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会概率性挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: T6481655.java
 * -@ExecuteClass: T6481655
 * -@ExecuteArgs:
 * -@Remark:
 */

public class T6481655 {

    public static <T> T getT(T t) {
        return t;
    }

    public static void main(String... s) {
        if (T6481655.getT("").equals("") && (T6481655.getT("")).getClass().isInstance("java.lang.String")) {
            System.out.println("ExpectResult");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
