/*
 * -@TestCaseID: ConstCharAppend
 * -@TestCaseName: ConstCharAppend
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: ConstCharAppend.java
 * -@ExecuteClass: ConstCharAppend
 * -@ExecuteArgs:
 * -@Remark:
 */

public class ConstCharAppend {
    public static void main(String[] args) {
        if (!("" + 'a' + 'b').equals("ab")) {
            System.out.println("append of chars is wrong: 4103959");
        } else {
            System.out.println("ExpectResult");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
