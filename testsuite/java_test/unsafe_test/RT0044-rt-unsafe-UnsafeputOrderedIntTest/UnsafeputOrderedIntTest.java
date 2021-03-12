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
public class UnsafeputOrderedIntTest {
    private static int res = 99;
    private static Object obj = new Object();
    private static Long offset =0l;
    private static int flag =0;
    public static void main(String[] args) throws InterruptedException {
        System.out.println(run(args, System.out));
    }
    private static int run(String[] args, PrintStream out) throws InterruptedException {
        int result = 2/*STATUS_FAILED*/;
        try {
            result = UnsafeputOrderedIntTest_1();
        } catch(Exception e){
            e.printStackTrace();
            UnsafeputOrderedIntTest.res -= 2;
        }
//        System.out.println(flag);
        if (flag==0){
//            System.out.println("It is true");
        }
        Thread.sleep(4000);
//        System.out.println(flag);
//        System.out.println("result:" +result);
//        System.out.println("res: "+ res);
        if (result == 3 && UnsafeputOrderedIntTest.flag == 10){
            result = 0;
        }
        return result;
    }
    private static int UnsafeputOrderedIntTest_1(){
        int result = 3;
        Field field;
        try{
            field = Unsafe.class.getDeclaredField("theUnsafe");
            field.setAccessible(true);
            Unsafe unsafe = (Unsafe)field.get(null);
            Thread boyThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    Field param = null;
                    try {
                        param = Billie14.class.getDeclaredField("height");
                    } catch (NoSuchFieldException e) {
                        e.printStackTrace();
                    }
                    offset = unsafe.objectFieldOffset(param);
                    obj = new Billie14();
                    unsafe.putOrderedInt(obj, offset, 10);
                }
            });
            Thread girlThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    int result;
                    flag = unsafe.getInt(obj,offset);
//                    System.out.println("======================="+flag);
                    try {
                        Thread.sleep(3000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    flag = unsafe.getInt(obj,offset);
//                    System.out.println("======================="+flag);
                }
            });
            boyThread.start();
            girlThread.start();
        }catch (Exception e){
            e.printStackTrace();
            return 40;
        }
        return result;
    }
}
class Billie14{
    public int height = 8;
    private String[] color = {"black","white"};
    private String owner = "Me";
    private byte length = 0x7;
    private String[] water = {"day","wet"};
    private long weight = 100L;
    private volatile int age = 18;
    private volatile long birth = 20010214L;
    private volatile String lastname = "eilish";
}
