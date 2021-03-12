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


public class ThreadDMCountStackFrames1 extends Thread{
    static int eCnt = 0;
    public static void main(String[] args) {
        ThreadDMCountStackFrames1 thread_obj = new ThreadDMCountStackFrames1();
        thread_obj.start();
        try{
            sleep(50);
        }catch (InterruptedException e1) {
            System.err.println(e1);
        }
        if(thread_obj.countStackFrames() >= 0 && eCnt == 1) {
            System.out.println(0);
            return;
        }
        thread_obj.stop();
        System.out.println(2);
    }
    public void run() {
        synchronized (this) {
            try {
                this.suspend();
            } catch (UnsupportedOperationException e2) {
                eCnt ++;
            }
        }
    }
}
