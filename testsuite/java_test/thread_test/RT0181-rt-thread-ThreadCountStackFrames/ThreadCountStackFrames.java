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


public class ThreadCountStackFrames {
    static int threadFinish = 0;
    public static void main(String[] args) {
        int result = 2;
        Thread thread = new Thread("ThreadCountStackFrames") {
            public void run() {
                try{
                    sleep(1000);
                }catch(Exception e) {
                    System.out.println(e);
                }
                ThreadCountStackFrames.threadFinish++;
            }
        };
        // Before the thread started.
        if (thread.countStackFrames() == 0 && thread.getStackTrace().length == 0) {
            result--;
        }
        thread.start();
        try{
            thread.join();
        }catch(Exception e) {
            System.out.println(e);
        }
        // After a thread finished.
        if (thread.countStackFrames() == 0 && thread.getStackTrace().length == 0
                && ThreadCountStackFrames.threadFinish == 1) {
            result--;
        }
        System.out.println(result);
    }
}