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


import java.util.Arrays;
public class RC_newObjectToField_02 {
    final static String STR = "MapleTest";
    static int check = 0;
    volatile static String[] strArray;
    int[] intArray;
    static String str1;
    volatile String str2;
    private RC_newObjectToField_02() throws ArithmeticException{
        strArray=new String[10];
        str1=new String("str1")+"MapleTest";
        str2="FigoTest";
        intArray = new int[]{1/0};
    }
    private RC_newObjectToField_02(String str) throws StringIndexOutOfBoundsException{
        str1=(new String("str1")+str).substring(-1,-2);
        str2=new String("str2")+str;
    }
    private RC_newObjectToField_02(String[] str) throws ArrayIndexOutOfBoundsException{
        Arrays.sort(str);
        Arrays.toString(str);
        new Object();
        new String("Just Test");
        Arrays.binarySearch(str,"d");
        intArray=new int[10];
        strArray=new String[1];
        strArray[2]=new String("IndexOutBounds");
    }
    public static void main(String[] args){
        for(int i =1; i<=100 ; i++){
            test_new_objct_assign();
            if(check != 3){
                System.out.println("ErrorResult");
                break;
            }else{
                check=0;
                str1=null;
            }
        }
        System.out.println("ExpectResult");
    }
    private static void test_new_objct_assign(){
        String[] arrayStr = {"c","a","b"};
        try{
        RC_newObjectToField_02 rctest=new RC_newObjectToField_02(arrayStr);
        }catch(ArrayIndexOutOfBoundsException a) {
            check += 1;
        }
        try{
        RC_newObjectToField_02 rctest = new RC_newObjectToField_02("secondTimeTest");}
        catch(StringIndexOutOfBoundsException s){
            check +=1;
        }
        try{
            RC_newObjectToField_02 rctest=new RC_newObjectToField_02();}
        catch(ArithmeticException a){
            check +=1;
        }
    }
}
