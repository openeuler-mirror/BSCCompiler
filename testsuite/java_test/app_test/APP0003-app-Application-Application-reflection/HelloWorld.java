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

import java.lang.reflect.*;
class Pertnn {
    public Pertnn() {
      age = 1234;
    }
    public int getAge() {
        return age;
    }
    public void setAge(int age) {
        this.age = age;
    }
    public int age;
}

class Person {
    public Person() {
      System.out.println("Invoking Person Constructor!");
      age = 5678;
    }
    public int getAge() {
      System.out.println("Invoking Person getAge!");
      return age;
    }
    public void setAge(int age) {
        this.age = age;
    }
    public int age;
}
public final class HelloWorld {
  public static void main(String [] args) {
        System.out.println("HelloWorld!");			
        Class<?> demo=null;
        try{
            demo=Class.forName("Person");
        }catch (Exception e) {
            System.out.println("ex1");
        }

        try{
            Method method=demo.getMethod("getAge");
            Object o =	demo.newInstance();
            method.invoke(o);
            Field field = demo.getDeclaredField("age");
            long value = field.getLong(o);
            if(value == 5678)
              System.out.println("Good Value Before Setting!");			
        /*    field.setLong(o, 9876);  // This should throw exception
            value = field.getLong(o);
            if(value == 9876)
              System.out.println("Good Value After Setting!");	*/
        }catch (Exception e) {
            System.out.println("ex2");
        }
}
}

