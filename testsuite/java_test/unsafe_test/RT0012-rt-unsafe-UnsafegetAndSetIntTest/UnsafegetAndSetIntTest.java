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
public class UnsafegetAndSetIntTest {
    private static int res = 99;
    public static void main(String[] args){
        System.out.println(run(args, System.out));
    }
    private static int run(String[] args, PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            result = UnsafegetAndSetIntTest_1();
        } catch(Exception e){
            e.printStackTrace();
            UnsafegetAndSetIntTest.res -= 2;
        }
        if (result == 3 && UnsafegetAndSetIntTest.res == 97){
            result =0;
        }
        return result;
    }
    private static int UnsafegetAndSetIntTest_1(){
        Unsafe unsafe;
        Field field;
        Long offset;
        Field param;
        Object obj;
        int result;
        int result2;
        try{
            field = Unsafe.class.getDeclaredField("theUnsafe");
            field.setAccessible(true);
            unsafe = (Unsafe)field.get(null);
            param = Billie36.class.getDeclaredField("height");
            offset = unsafe.objectFieldOffset(param);
            obj = new Billie36();
            result = unsafe.getAndSetInt(obj, offset, 20);
//            System.out.println(result);
            result2 = unsafe.getInt(obj, offset);
//            System.out.println(result2);
            if(result == 8 && result2 == 20){
                UnsafegetAndSetIntTest.res -= 2;
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
class Billie36{
    public int height = 8;
    private String[] color = {"black","white"};
    private String owner = "Me";
    private byte length = 0x7;
    private String[] water = {"day","wet"};
    private long weight = 100L;
    private volatile int age = 18;
    private volatile long birth = 20010214L;
    private volatile String lastname = "eilish";
    private boolean gender  = false;
}
