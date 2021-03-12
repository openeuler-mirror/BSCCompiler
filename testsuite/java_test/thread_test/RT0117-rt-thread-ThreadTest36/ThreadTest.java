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


public class ThreadTest {
    public static void main(String[] args) throws Exception {
        /**
         * Verify the setPriority() method with new priority higher
         * than the maximum permitted priority for the thread's group.
        */

        ThreadGroup tg = new ThreadGroup("group1");
        int maxTGPriority = Thread.MAX_PRIORITY - 1;
        tg.setMaxPriority(maxTGPriority);
        Thread t = new Thread(tg, new ThreadRun("helloworld!"));
        t.setPriority(Thread.MAX_PRIORITY);
        System.out.println(maxTGPriority);
        System.out.println(tg.getMaxPriority() + " -- " + Thread.MAX_PRIORITY + " -- " + t.getPriority());
        /**
         * Verify the setPriority() method with new priority lower
         * than the current one.
        */

        Thread tt = new Thread();
        int p = tt.getPriority();
        int newPriority = p - 1;
        if (newPriority >= Thread.MIN_PRIORITY) {
            tt.setPriority(newPriority);
            System.out.println(newPriority + " === " + tt.getPriority());
        }
        /**
         * Verify the setPriority() method with new priority out of the legal range.
        */

        Thread ttt = new Thread();
        try {
            ttt.setPriority(Thread.MAX_PRIORITY + 2);
            System.out.println("Fail -- IllegalArgumentException should be thrown when new priority out of the legal range");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException has be thrown when new priority out of the legal range");
        }
        /**
         * Verify the setPriority() method to a dead thread. do nothing
        */

        Thread t4 = new Thread(tg, new ThreadRun("helloworld2"));
        t4.start();
        t4.join();
        System.out.println(t4.getState());
        System.out.println(t4.getPriority());
        t4.setPriority(Thread.MAX_PRIORITY);
        System.out.println(t4.getPriority());
    }
    // Test for setPriority()
    private static class ThreadRun implements Runnable {
        private final String helloWorld;
        public ThreadRun(String str) {
            helloWorld = str;
        }
        public void run() {
            System.out.println(helloWorld);
        }
    }
}