/*
 * -@TestCaseID: WrongNew
 * -@TestCaseName: WrongNew
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会概率性挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: WrongNew.java
 * -@ExecuteClass: WrongNew
 * -@ExecuteArgs:
 * -@Remark:
 */

class WrongNewList<T> {
}

class WrongNewArrayList<T> extends WrongNewList<T> {
}

public class WrongNew {
    public static void main(String[] ps) {
        WrongNewList<String> list = getList();
        if (list != null) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }

    public static WrongNewList<String> getList() {
        return new WrongNewArrayList();
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
