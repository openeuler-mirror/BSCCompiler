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

import java.lang.Thread;

public class ThreadHelloWorld implements Runnable {

  private final String helloworld;

  public static void main(String args[]) throws InterruptedException {
    Thread thread = new Thread(new ThreadHelloWorld("Hello, World"));
    thread.start();
    thread.join();
  }

  public ThreadHelloWorld(String str){
    helloworld = str;
  }
  
  public void run(){
    System.out.println(helloworld);
  }
}


