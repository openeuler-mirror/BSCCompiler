/*
 * -@TestCaseID:Alloc_Threadm_192x8B_2
 * -@TestCaseName:MyselfClassName
 * -@RequirementName:[运行时需求]支持自动内存管理
 * -@Title:ROS Allocator is in charge of applying and releasing objects.This mulit thread testcase is mainly for testing
 *         objects from 128*8B to 192*8B(max) with 6 threads and time sleep
 * -@Condition: no
 * -#c1
 * -@Brief:functionTest
 * -#step1
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: Alloc_Threadm_192x8B_2.java
 * -@ExecuteClass: Alloc_Threadm_192x8B_2
 * -@ExecuteArgs:
 * -@Remark:
 */

import java.util.ArrayList;

class Alloc_Threadm_192x8B_2_01 extends Thread {
    private static final int PAGE_SIZE = 4 * 1024;
    private static final int OBJ_HEADSIZE = 8;
    private static final int MAX_192_8B = 192 * 8;
    private static boolean checkout = false;

    public void run() {
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            // do nothing
        }
        int checkSize = 0;
        for (int i = 1024 - OBJ_HEADSIZE; i <= MAX_192_8B - OBJ_HEADSIZE; i = i + 100) {
            for (int j = 0; j < (PAGE_SIZE * 512 / (i + OBJ_HEADSIZE) + 10); j++) {
                if (j % 1000 == 0) {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        // do nothing
                    }
                }
                temp = new byte[i];
                store.add(temp);
                checkSize += store.size();
                store = new ArrayList<byte[]>();
            }
        }
        if (checkSize == 10117) {
            checkout = true;
        }
    }

    public boolean check() {
        return checkout;
    }
}

public class Alloc_Threadm_192x8B_2 {
    public static void main(String[] args) {
        Runtime.getRuntime().gc();
        Alloc_Threadm_192x8B_2_01 test1 = new Alloc_Threadm_192x8B_2_01();
        test1.start();
        Alloc_Threadm_192x8B_2_01 test2 = new Alloc_Threadm_192x8B_2_01();
        test2.start();
        Alloc_Threadm_192x8B_2_01 test3 = new Alloc_Threadm_192x8B_2_01();
        test3.start();
        Alloc_Threadm_192x8B_2_01 test4 = new Alloc_Threadm_192x8B_2_01();
        test4.start();
        try {
            test1.join();
            test2.join();
            test3.join();
            test4.join();
        } catch (InterruptedException e) {
            // do nothing
        }
        if (test1.check() == true && test2.check() == true && test3.check() == true && test4.check() == true) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("Error");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
