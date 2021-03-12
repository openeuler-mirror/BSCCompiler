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


import sun.misc.Unsafe;
import java.io.PrintStream;
import java.lang.reflect.Field;
public class UnsafearrayIndexScaleTest {
    private static int res = 99;
    public static void main(String[] args){
        System.out.println(run(args, System.out));
    }
    private static int run(String[] args, PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            result = UnsafearrayIndexSacleTest_1();
        } catch(Exception e){
            e.printStackTrace();
            UnsafearrayIndexScaleTest.res = UnsafearrayIndexScaleTest.res - 10;
        }
//        System.out.println(result);
//        System.out.println(UnsafearrayBaseOffsetTest.res);
        if (result == 3 && UnsafearrayIndexScaleTest.res == 97){
            result =0;
        }
        return result;
    }
    private static int UnsafearrayIndexSacleTest_1(){
        Unsafe unsafe;
        Field field;
        Object obj;
        long scale;
        long offset;
        long result;
        try{
            field = Unsafe.class.getDeclaredField("theUnsafe");
            field.setAccessible(true);
            unsafe = (Unsafe)field.get(null);
            obj = new long[]{9, 8, 7, 6};
            scale =  unsafe.arrayIndexScale(obj.getClass());
            offset = unsafe.arrayBaseOffset(obj.getClass());
            result = unsafe.getLong(obj, offset + 2*scale);
            if(result == 7){
                UnsafearrayIndexScaleTest.res -= 2;
            }
        }catch (NoSuchFieldException e){
            e.printStackTrace();
            return 40;
        }catch (IllegalAccessException e){
            e.printStackTrace();
            return 41;
        }
        return 3;
    }
}