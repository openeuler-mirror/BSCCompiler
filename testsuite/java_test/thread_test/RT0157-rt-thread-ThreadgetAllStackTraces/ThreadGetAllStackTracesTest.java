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


import java.util.Map;
public class ThreadGetAllStackTracesTest extends Thread {
    static volatile boolean keeprun = true;
    static class MyThread extends Thread {
        MyThread(String name) {
            super(name);
        }
        public void run() {
            while (keeprun) {
                try {
                    Thread.sleep(100);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }
    public static void main(String[] args) {
        Thread test = new MyThread("haha");
        Map m = getAllStackTraces();
        String stOrigin = m.keySet().toString();
        int sizeOrigin = m.size();
        if (stOrigin.contains("haha")) {
            System.out.println(1);
            return;
        }
        test.start();
        m = getAllStackTraces();
        String stRun = m.keySet().toString();
        int sizeRun = m.size();
        if (!stRun.contains("haha") || (sizeRun - sizeOrigin) != 1) {
            System.out.println(2);
            return;
        }
        keeprun = false;
        try {
            test.join();
        } catch (Throwable e) {
            e.printStackTrace();
        }
        m = getAllStackTraces();
        String stFinish = m.keySet().toString();
        int sizeFinish = m.size();
        if (stFinish.contains("haha") || (sizeRun - sizeFinish) != 1) {
            System.out.println(3);
            return;
        }
        System.out.println(0);
    }
}
