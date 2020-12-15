/*
 * -@TestCaseID:Alloc_192x8B
 * -@TestCaseName:Alloc_192x8B
 * -@RequirementName:[运行时需求]支持自动内存管理
 * -@Title:ROS Allocator is in charge of applying and releasing objects.This testcase is mainly for testing big objects from 128*8B to 192*8B(max)
 * -@Condition: no
 * -#c1
 * -@Brief:ROS Allocator is in charge of applying and releasing objects.This testcase is mainly for testing big objects from 128*8B to 192*8B(max)
 * -#step1
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: Alloc_192x8B.java
 * -@ExecuteClass: Alloc_192x8B
 * -@ExecuteArgs:
 * -@Remark:
 */
import java.util.ArrayList;

public class Alloc_192x8B {
    private  static final int PAGE_SIZE = 4*1024;
    private static final int OBJ_HEADSIZE = 8;
    private static final int MAX_192_8B = 192*8;
    private static ArrayList<byte[]> store;

    private static int alloc_test(int slot_type) {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;

        for (int i = 1024 + 1 - OBJ_HEADSIZE; i <= slot_type - OBJ_HEADSIZE; i = i + 6) {
            for (int j = 0; j < (PAGE_SIZE * 1280 / (i + OBJ_HEADSIZE) + 5); j++) {
                temp = new byte[i];
                store.add(temp);
            }
            sum += store.size();
            store = new ArrayList<byte[]>();
        }
        return sum;
    }

    public static void main(String[] args) {
        store = new ArrayList<byte[]>();
        int result = alloc_test(MAX_192_8B);
        if ( result == 357534) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("Error");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
