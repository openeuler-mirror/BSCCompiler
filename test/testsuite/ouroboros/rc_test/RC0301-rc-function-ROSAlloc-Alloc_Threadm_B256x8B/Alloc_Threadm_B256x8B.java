/*
 *- @TestCaseID:Alloc_Threadm_B256x8B
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title:ROS Allocator is in charge of applying and releasing objects.This mulit thread testcase is mainly for testing objects from 1MB to 2MB(max) with 6 threads
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: Alloc_Threadm_B256x8B.java
 *- @ExecuteClass: Alloc_Threadm_B256x8B
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.util.ArrayList;

class Alloc_Threadm_B256x8B_01 extends Thread {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_B256_8B=2*1024*1024;
    private static boolean checkout=false;

    public void run() {
        ArrayList<byte[]> store=new ArrayList<byte[]>();
        byte[] temp;
        int check_size=0;
        for(int i=1024*1024-OBJ_HEADSIZE;i<=MAX_B256_8B-OBJ_HEADSIZE;i=i+15120)
        {
            for(int j=0;j<(PAGE_SIZE*10240/(i+OBJ_HEADSIZE)+10);j++)
            {
                temp = new byte[i];
                store.add(temp);
                check_size +=store.size();
                store=new ArrayList<byte[]>();
            }
        }

        //System.out.println(check_size);
        if(check_size == 2612)
            checkout=true;

    }
    public boolean check() {
        return checkout;
    }

}

public class Alloc_Threadm_B256x8B {
    public static void main(String[] args){
        Alloc_Threadm_B256x8B_01 test1=new Alloc_Threadm_B256x8B_01();
        test1.start();
        try{ test1.sleep(1000);} catch(InterruptedException e){}
        Alloc_Threadm_B256x8B_01 test2=new Alloc_Threadm_B256x8B_01();
        test2.start();
        try{ test2.sleep(1000);} catch(InterruptedException e){}
        Alloc_Threadm_B256x8B_01 test3=new Alloc_Threadm_B256x8B_01();
        test3.start();
        try{ test3.sleep(1000);} catch(InterruptedException e){}
        Alloc_Threadm_B256x8B_01 test4=new Alloc_Threadm_B256x8B_01();
        test4.start();
        try{ test4.sleep(1000);} catch(InterruptedException e){}
        Alloc_Threadm_B256x8B_01 test5=new Alloc_Threadm_B256x8B_01();
        test5.start();
        try{ test5.sleep(1000);} catch(InterruptedException e){}
        Alloc_Threadm_B256x8B_01 test6=new Alloc_Threadm_B256x8B_01();
        test6.start();
        try{
            test1.join();
            test2.join();
            test3.join();
            test4.join();
            test5.join();
            test6.join();
        }catch (InterruptedException e){}
        if(test1.check()==true&&test2.check()==true && test3.check()==true&&test4.check()==true && test5.check()==true&&test6.check()==true)
            System.out.println("ExpectResult");
        else
            System.out.println("Error");

    }

}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
