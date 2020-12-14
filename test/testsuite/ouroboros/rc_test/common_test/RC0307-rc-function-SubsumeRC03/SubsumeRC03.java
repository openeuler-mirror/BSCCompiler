/*
 *- @TestCaseID:maple/runtime/rc/function/SubsumeRC03.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test subsumerc for RC, assign another object's field to a field
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: SubsumeRC03.java
 *- @ExecuteClass: SubsumeRC03
 *- @ExecuteArgs:
 *- @Remark:
 *- @ExpectMplCode:
 *- callassigned &MRT_LoadRefField_NaiveRCFast (
 *-   regread ref %2,
 *-     iaddrof ref <* <$LSubsumeRC03_3B>> 4 (regread ref %2)) { regassign ref %1}
 *- call &CC_WriteRefFieldNoInc (
 *-   regread ref %3,
 *-   iaddrof ref <* <$LSubsumeRC03_3B>> 4 (regread ref %3),
 *-   regread ref %1)
 */
public class SubsumeRC03 {
    private String str;

    public SubsumeRC03() {
    }

    public void testfunc(SubsumeRC03 t) {
        str = t.str;
        return ;
    }

    public static void main(String[] args){
        SubsumeRC03 temp0 = new SubsumeRC03();
        SubsumeRC03 temp1 = new SubsumeRC03();
        temp1.testfunc(temp0);
        System.out.println("ExpectResult");
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
