/*
 * -@TestCaseID: SingleLocalTest
 * -@TestCaseName: SingleLocalTest
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会概率性挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: SingleLocalTest.java
 * -@ExecuteClass: SingleLocalTest
 * -@ExecuteArgs:
 * -@Remark:
 */

public class SingleLocalTest {
    static F f;

    public static void main(String[] args) {
        StringBuffer sb = new StringBuffer();
        class Local1 {
            public Local1() {
                f = () -> new Local1();
                sb.append("1");
            }
        }
        new Local1();
        f.f();
        String s = sb.toString();
        if (!s.equals("11")) {
            System.out.println("Expected '11' got '" + s + "'");
        } else {
            System.out.println("ExpectResult");
        }
    }

    interface F {
        void f();
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
