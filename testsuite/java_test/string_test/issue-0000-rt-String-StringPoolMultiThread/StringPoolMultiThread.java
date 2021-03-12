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
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
public class StringPoolMultiThread {
    static int res = 99;
    // 3000/thread  48000/16thread   ==> 1000/mapleStringPool
    static int stringCountForEveryThread=3000;
    static int threadCount=16;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[],PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            result = StringPoolMultiThread_1();
        } catch(Exception e){
            System.out.println(e);
            StringPoolMultiThread.res = StringPoolMultiThread.res - 10;
        }
//        System.out.println(result);
//        System.out.println(StringPoolMultiThread.res);
        if (result == 3 && StringPoolMultiThread.res == 97){
            result =0;
        }
        return result;
    }
    public static  int StringPoolMultiThread_1() throws InterruptedException, ExecutionException {
        int result1 = 4; /*STATUS_FAILED*/
        int length = StringPoolMultiThread.stringCountForEveryThread;// 3000
        int threadn = StringPoolMultiThread.threadCount; //16
        for (int i=0;i<length;i++) {
            ExecutorService executorService = Executors.newFixedThreadPool(threadn);
            Object res[] = new Object[threadn];
            for (int j = 0; j < threadn; j++) {
                /* 启动线程时会返回一个Future对象,可以通过future对象获取现成的返回值。
                 * 在执行future.get()时，主线程会堵塞，直至当前future线程返回结果。
                */

                res[j]=executorService.submit(new ThreadWithCallback(i,j)).get();
//                System.out.println("============ result in thread: "+res[j]);
            }
            executorService.shutdown();
            while (true) {
                if (executorService.isTerminated()) {
                    break;
                }
                Thread.sleep(1000);
            }
            for (int k = 0; k < threadn-1; k++) {
//                System.out.println("== return =="+res[k]);
//                System.out.println("== return =="+res[k+1]);
                if (res[k] != res[k+1]){
                    return 10;
                }else {
                    continue;
                }
            }
        }
        StringPoolMultiThread.res = StringPoolMultiThread.res - 2;
        return 3;//PASS
    }
    static class ThreadWithCallback implements Callable {
        private final int name;
        private final int count;
        private String result;
        public ThreadWithCallback(int count,int name) {
            this.name = name;
            this.count =count;
        }
        @Override
        //相当于Thread的run方法
        public Object call() {
//            System.out.println("== threadname: "+name+"  == count:"+count + " ==");
            try {
                String  s= Integer.toHexString(count);
                result=s.intern();
//                System.out.println("== result in run: "+result);
            } catch (Exception e) {
                System.out.println(e);
            }
            return result;
        }
    }
}
