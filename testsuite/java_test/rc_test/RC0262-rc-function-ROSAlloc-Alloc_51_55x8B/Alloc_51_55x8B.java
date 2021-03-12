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
public class Alloc_51_55x8B {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_51_8B=51*8;
    private final static int MAX_52_8B=52*8;
    private final static int MAX_53_8B=53*8;
    private final static int MAX_54_8B=54*8;
    private final static int MAX_55_8B=55*8;
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
            for(int j=0;j<(PAGE_SIZE*6/(i+OBJ_HEADSIZE)+10);j++)
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
        int countSize51 = alloc_test(MAX_51_8B);
        int countSize52 = alloc_test(MAX_52_8B);
        int countSize53 = alloc_test(MAX_53_8B);
        int countSize54 = alloc_test(MAX_54_8B);
        int countSize55 = alloc_test(MAX_55_8B);
        //System.out.println(countSize51);
        //System.out.println(countSize52);
        //System.out.println(countSize53);
        //System.out.println(countSize54);
        //System.out.println(countSize55);
        if (countSize51 == 562 && countSize52 == 553 && countSize53 == 543 && countSize54 == 535 && countSize55 == 526)
            System.out.println("ExpectResult");
        else
            System.out.println("Error");
    }
}
