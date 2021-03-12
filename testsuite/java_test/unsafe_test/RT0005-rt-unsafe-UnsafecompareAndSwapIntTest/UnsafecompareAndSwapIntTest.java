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
public class UnsafecompareAndSwapIntTest {
    private static int res = 99;
    public static void main(String[] args){
        System.out.println(run(args, System.out));
    }
    private static int run(String[] args, PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            result = UnsafecompareAndSwapIntTest_1();
        } catch(Exception e){
            e.printStackTrace();
            UnsafecompareAndSwapIntTest.res = UnsafecompareAndSwapIntTest.res - 10;
        }
//        System.out.println(result);
//        System.out.println(UnsafearrayBaseOffsetTest.res);
        if (result == 3 && UnsafecompareAndSwapIntTest.res == 97){
            result =0;
        }
        return result;
    }
    private static int UnsafecompareAndSwapIntTest_1(){
        Unsafe unsafe;
        Field field;
        Field param;
        Object obj;
        long scale;
        long offset;
        int result;
        try{
            field = Unsafe.class.getDeclaredField("theUnsafe");
            field.setAccessible(true);
            unsafe = (Unsafe)field.get(null);
            obj = new Billie3();
            param = Billie3.class.getDeclaredField("height");
            offset = unsafe.objectFieldOffset(param);
            unsafe.compareAndSwapInt(obj, offset, 8, 10);
            result = unsafe.getInt(obj, offset);
            if(result == 10){
                UnsafecompareAndSwapIntTest.res -= 2;
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
class Billie3{
    public int height = 8;
    private String[] color = {"black","white"};
    private String owner = "Me";
    private byte length = 0x7;
    private String[] water = {"day","wet"};
}
