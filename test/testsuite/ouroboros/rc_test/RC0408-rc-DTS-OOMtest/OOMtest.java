/*
 *- @TestCaseID:OOMtest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: OOMtest.java
 *- @ExecuteClass: OOMtest
 *- @ExecuteArgs:
 *
 */

import java.util.ArrayList;

public class OOMtest {
    private static ArrayList<byte[]> store;

    private static int alloc_test() {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;

        for (int i = 1024 * 1024 * 10; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }

    public static void main(String[] args) {
        try {
            int result = alloc_test();
        } catch (OutOfMemoryError o) {
            System.out.println("ExpectResult");
        }
        if (store == null) {
            System.out.println("Error");
        } else {
			System.out.println("ExpectResult");
		}
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
