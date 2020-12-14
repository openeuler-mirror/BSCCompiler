/*
 * -@TestCaseID: RelaxedArrays
 * -@TestCaseName: RelaxedArrays
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: RelaxedArrays.java
 * -@ExecuteClass: RelaxedArrays
 * -@ExecuteArgs:
 * -@Remark:
 */

import java.util.ArrayList;
import java.util.List;

public class RelaxedArrays {
    static <T> T select(T... tl) {
        return tl.length == 0 ? null : tl[tl.length - 1];
    }

    public static void main(String[] args) {
        List<String>[] a = new StringList[20];
        if (select("A", "B", "C") != "C") {
            System.out.println("error");
        } else {
            System.out.println("ExpectResult");
        }
    }

    static class StringList extends ArrayList<String> {
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
