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
public class GetAllStackTraceIterating extends Thread {
    private static String[] methodNames = {"run", "A", "B", "C", "Done"};
    private static Object obj = new Object();
    private static Thread[] tt = new Thread[5];
    public static void main(String[] args) throws InterruptedException {
        int result1 = 1; /* STATUS_FAILED*/

        Map map = getAllStackTraces();
        String stOrigin = map.keySet().toString();
        int sizeOrigin = map.size();
        for (int i = 0; i < tt.length; i++) {
            tt[i] = new ThreadOne();
        }
        for (int i = 0; i < tt.length; i++) {
            tt[i].start();
            map = getAllStackTraces();
            String stRun = map.keySet().toString();
            int sizeRun = map.size();
            if ((sizeRun - sizeOrigin) == 5 && stRun.contains("main")) {
                result1--;
            }
        }
        for (int i = 0; i < tt.length; i++) {
            try {
                tt[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        System.out.println(result1);
    }
    static class ThreadOne extends Thread {
        @Override
        public void run() {
            System.out.println("0");
            A();
        }
        private void A() {
            waitForDump();
            B();
        }
        private void B() {
            waitForDump();
            C();
        }
        private void C() {
            waitForDump();
            Done();
        }
        private void Done() {
            waitForDump();
        }
    }
    static private void waitForDump() {
        synchronized (obj) {
            try {
                obj.wait(200);
            } catch (InterruptedException e) {
                System.out.println("wait is interrupted");
            }
        }
    }
}
// EXEC:%maple  GetAllStackTraceIterating.java -p %platform %build_option -o %n.so
// EXEC:%run -b %host -l %username -p %port --run_type %run_type --sn %sn  %n.so GetAllStackTraceIterating  %mplsh_option | compare %f
// ASSERT: scan 0\n0\n0\n0\n0\n0\n
