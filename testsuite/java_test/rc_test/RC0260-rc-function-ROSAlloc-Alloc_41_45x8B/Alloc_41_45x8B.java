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
public class Alloc_41_45x8B {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_41_8B=41*8;
    private final static int MAX_42_8B=42*8;
    private final static int MAX_43_8B=43*8;
    private final static int MAX_44_8B=44*8;
    private final static int MAX_45_8B=45*8;
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
            for(int j=0;j<(PAGE_SIZE*5/(i+OBJ_HEADSIZE)+10);j++)
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
        int countSize41 = alloc_test(MAX_41_8B);
        int countSize42 = alloc_test(MAX_42_8B);
        int countSize43 = alloc_test(MAX_43_8B);
        int countSize44 = alloc_test(MAX_44_8B);
        int countSize45 = alloc_test(MAX_45_8B);
        //System.out.println(countSize41);
        //System.out.println(countSize42);
        //System.out.println(countSize43);
        //System.out.println(countSize44);
        //System.out.println(countSize45);
        if (countSize41 == 581 && countSize42 == 569 && countSize43 == 557 && countSize44 == 547 && countSize45 == 536)
            System.out.println("ExpectResult");
        else
            System.out.println("Error");
    }
}
