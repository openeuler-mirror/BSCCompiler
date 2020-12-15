/*
 *- @TestCaseID:Alloc_31_35x8B
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title:ROS Allocator is in charge of applying and releasing objects.This testcase is mainly for testing objects from 31*8B to 35*8B(max)
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: Alloc_31_35x8B.java
 *- @ExecuteClass: Alloc_31_35x8B
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.util.ArrayList;

public class Alloc_31_35x8B {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_31_8B=31*8;
    private final static int MAX_32_8B=32*8;
    private final static int MAX_33_8B=33*8;
    private final static int MAX_34_8B=34*8;
    private final static int MAX_35_8B=35*8;
    private static ArrayList<byte[]> store;

    private static int alloc_test(int slot_type){
        store=new ArrayList<byte[]>();
        byte[] temp;
        int i;
        if(slot_type==24){
            i=1;}
        else if(slot_type==1024){
            i=64*8+1-OBJ_HEADSIZE;
        }else{
            i=slot_type-2*8+1;
        }

        for(;i<=slot_type-OBJ_HEADSIZE;i++)
        {
            for(int j=0;j<(PAGE_SIZE*4/(i+OBJ_HEADSIZE)+10);j++)
            {
                temp = new byte[i];
                store.add(temp);
            }
        }
        int check_size=store.size();
        store=new ArrayList<byte[]>();
        return check_size;
    }
    public static void main(String[] args) {
        store = new ArrayList<byte[]>();
        int countSize31 = alloc_test(MAX_31_8B);
        int countSize32 = alloc_test(MAX_32_8B);
        int countSize33 = alloc_test(MAX_33_8B);
        int countSize34 = alloc_test(MAX_34_8B);
        int countSize35 = alloc_test(MAX_35_8B);
        //System.out.println(countSize31);
        //System.out.println(countSize32);
        //System.out.println(countSize33);
        //System.out.println(countSize34);
        //System.out.println(countSize35);

        if (countSize31 == 612 && countSize32 == 596 && countSize33 == 580 && countSize34 == 564 && countSize35 == 550)
            System.out.println("ExpectResult");
        else
            System.out.println("Error");
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
