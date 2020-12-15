/*
 *- @TestCaseID:Alloc_Thread_128x8B
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title:ROS Allocator is in charge of applying and releasing objects.This mulit thread testcase is mainly for testing objects from 64*8B to 128*8B(max)
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: Alloc_Thread_128x8B.java
 *- @ExecuteClass: Alloc_Thread_128x8B
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.util.ArrayList;

class Alloc_Thread_128x8B_01 extends Thread {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_128_8B=128*8;
    private static boolean checkout=false;

    public void run() {
        ArrayList<byte[]> store=new ArrayList<byte[]>();
        byte[] temp;
        int check_size=0;
        for(int i=512-OBJ_HEADSIZE;i<=MAX_128_8B-OBJ_HEADSIZE;i=i+100)
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
        if(check_size == 348510)
            checkout=true;

    }
    public boolean check() {
        return checkout;
    }

}

public class Alloc_Thread_128x8B {
    public static void main(String[] args){
        Alloc_Thread_128x8B_01 test1=new Alloc_Thread_128x8B_01();
        test1.start();
        try{
            test1.join();
        }catch (InterruptedException e){}
        if(test1.check()==true)
            System.out.println("ExpectResult");
        else
            System.out.println("Error");

    }

}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
