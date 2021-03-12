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


public class ThreadDMdestory2 {
    public static void main(String[] args) {
        System.out.println(run());
    }
    private static int run() {
        int exceptionCount = 0;
        // destroy001: test destroy() when the thread is not started
        try {
            new Thread().destroy();
            return 1;
        } catch (UnsupportedOperationException ok) {
            exceptionCount++;
        }
        // destroy002: test destroy() when the thread is finished
        Thread thread = new Thread();
        thread.start();
        try {
            thread.join();
        } catch (InterruptedException e) {
            System.out.println(e);
        }
        try {
            thread.destroy();
            return 2;
        } catch (UnsupportedOperationException ok) {
            exceptionCount++;
        }
        // destroy003: test destroy() when a thread is sleeping
        SleepingThread st = new SleepingThread();
        st.start();
        while (! st.started){
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                // ignore any attempt to interrupt
                System.out.println(e);
            }
        }
        try {
            st.destroy();
            return 3;
        } catch (UnsupportedOperationException ok) {
            exceptionCount++;
        }
        st.checked=true;
        if (exceptionCount == 3) {
            return 0;
        }else{
            return 4;
        }
    }
}
class SleepingThread extends Thread {
    public volatile boolean started = false;
    public volatile boolean checked = false;
    public void run() {
        started = true;
        while (! checked){
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                // ignore any attempt to interrupt
                System.out.println(e);
            }
        }
    }
}
