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


import java.io.PrintStream;
public class StringOOMTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[],PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            result = result-StringOOMTest_1();
//            result = result-StringOOMTest_2(); time out
        } catch(Exception e){
            System.out.println(e);
            processResult = processResult - 10;
        }
//        System.out.println(result);
//        System.out.println(StringOOMTest.res);
        if (result == 1 && processResult == 97){
            result =0;
        }
        return result;
    }
    public static  int StringOOMTest_1() {
        int result1 = 4; /*STATUS_FAILED*/
        int length=1024*1024*5120;// 1024*1024*1B=1M,*512=512M,set as 1024*1024*5120
        try {
            String[] s;
            s = new String[ length ];
            s [0] = "a" ;//if no this,y = new int[] will be optimizated and deleted.
//            System.out.println("s: "+ Arrays.toString(s));
        } catch (OutOfMemoryError e) {
            processResult = processResult - 2;
            return 1;
        }
        processResult = processResult - 10;
        return 40;
    }
    public static  int StringOOMTest_2() {
        int result1 = 4; /*STATUS_FAILED*/
        int loop = 2147483647; // set as 2147483647
//        String[] s= new String[loop];
        String s=null;
        while(loop >0 ) {
            loop--;
            try {
                s = new String(Integer.toBinaryString(loop));
            } catch (OutOfMemoryError e) {
                processResult = processResult - 2;
                return 1;
            }
        }
        System.out.println("s: "+ s);
        processResult = processResult - 10;
        return 40;
    }
}
