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


public class RC_staticInput {
    static int []test= {10,20,30,40};
    public static void main(String [] args) {
        if(test.length==4)
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
        test(4,test);
        if(test.length==4)
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
    }
    public static void test(int first, int[] second) {
        int [] xyz = {23,24,25,26};
        test = xyz;
        if(second.length==4)
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
    }
}
