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
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.TreeSet;
public class UnsafeputIntVolatileTest {
    private static int res = 99;
    public static void main(String[] args){
        System.out.println(run(args, System.out));
    }
    private static int run(String[] args, PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            result = UnsafeputIntVolatileTest_1();
        } catch(Exception e){
            e.printStackTrace();
            UnsafeputIntVolatileTest.res -= 2;
        }
        if (result == 3 && UnsafeputIntVolatileTest.res == 97){
            result =0;
        }
        return result;
    }
    private static int UnsafeputIntVolatileTest_1()  {
        Unsafe unsafe;
        Field field;
        Long offset;
        Field param;
        Object obj;
        int result;
        try{
            field = Unsafe.class.getDeclaredField("theUnsafe");
            field.setAccessible(true);
            unsafe = (Unsafe)field.get(null);
            param = Billie7.class.getDeclaredField("age");
            offset = unsafe.objectFieldOffset(param);
            obj = new Billie7();
            result = unsafe.getIntVolatile(obj, offset);
            unsafe.putIntVolatile(obj, offset, 20);
            result = unsafe.getIntVolatile(obj, offset);
            if(result == 20){
                UnsafeputIntVolatileTest.res -= 2;
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
class Billie7{
    public int height = 8;
    private String[] color = {"black","white"};
    private String owner = "Me";
    private byte length = 0x7;
    private String[] water = {"day","wet"};
    private long weight = 100L;
    private volatile int age = 18;
}
