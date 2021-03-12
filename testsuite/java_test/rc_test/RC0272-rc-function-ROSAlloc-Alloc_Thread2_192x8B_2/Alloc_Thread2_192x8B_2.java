/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.util.ArrayList;
class Alloc_Thread2_192x8B_2_01 extends Thread {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_192_8B=192*8;
    private static boolean checkout=false;
    public void run() {
        ArrayList<byte[]> store=new ArrayList<byte[]>();
        try{Thread.sleep(100);}catch(InterruptedException e){}
        byte[] temp;
        int check_size=0;
        for(int i=1024-OBJ_HEADSIZE;i<=MAX_192_8B-OBJ_HEADSIZE;i=i+100)
        {
            for(int j=0;j<(PAGE_SIZE*10240/(i+OBJ_HEADSIZE)+10);j++)
            {
                if(j%10000==0)
                {
                    try{Thread.sleep(100);}catch(InterruptedException e){}
                }
                temp = new byte[i];
                store.add(temp);
                check_size +=store.size();
                store=new ArrayList<byte[]>();
            }
        }
        //System.out.println(check_size);
        if(check_size == 201256)
            checkout=true;
    }
    public boolean check() {
        return checkout;
    }
}
public class Alloc_Thread2_192x8B_2 {
    public static void main(String[] args){
        Alloc_Thread2_192x8B_2_01 test1=new Alloc_Thread2_192x8B_2_01();
        test1.start();
        Alloc_Thread2_192x8B_2_01 test2=new Alloc_Thread2_192x8B_2_01();
        test2.start();
        try{
            test1.join();
            test2.join();
        }catch (InterruptedException e){}
        if(test1.check()==true &&test2.check()==true )
            System.out.println("ExpectResult");
        else
            System.out.println("Error");
    }
}
