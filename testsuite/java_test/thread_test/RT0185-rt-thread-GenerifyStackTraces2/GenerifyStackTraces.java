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
public class GenerifyStackTraces {
    private static Object go = new Object();
    private static String[] methodNames = {"run", "A", "B", "C", "Done"};
    private static int DONE_DEPTH = 5;
    private static boolean testFailed = false;
    private static Thread one;
    private static boolean trace = false;
    public static void main(String[] args) throws Exception {
        if (args.length > 0 && args[0].equals("trace")) {
            trace = true;
        }
        one = new ThreadOne();
        one.start();
        DumpThread dt = new DumpThread();
        dt.start();
        try {
            one.join();
        } finally {
            dt.shutdown();
        }
        if (testFailed) {
            throw new RuntimeException("Test Failed.");
        }
    }
    static class DumpThread extends Thread {
        private volatile boolean finished = false;
        public void run() {
            int depth = 2;
            while (!finished) {
                // At each iterator, wait until ThreadOne blocks to wait for thread dump.
                // Then dump stack trace and notify ThreadOne to continue.
                try {
                    sleep(2000);
                    dumpStacks(depth);
                    depth++;
                    finishDump();
                } catch (Exception e) {
                    e.printStackTrace();
                    testFailed = true;
                }
            }
        }
        public void shutdown() throws InterruptedException {
            finished = true;
            this.join();
        }
    }
    static class ThreadOne extends Thread {
        public void run() {
	    System.out.println(0);
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
            // Get stack trace of current thread
            StackTraceElement[] stack = getStackTrace();
            try {
                checkStack(this, stack, DONE_DEPTH);
            } catch (Exception e) {
                e.printStackTrace();
                testFailed = true;
            }
        }
    }
    static private void waitForDump() {
        synchronized(go) {
            try {
               go.wait();
            } catch (Exception e) {
               throw new RuntimeException("Unexpected exception" + e);
            }
        }
    }
    static private void finishDump() {
        synchronized(go) {
            try {
               go.notifyAll();
            } catch (Exception e) {
               throw new RuntimeException("Unexpected exception" + e);
            }
        }
    }
    public static void dumpStacks(int depth) throws Exception {
        // Get stack trace of another thread
        StackTraceElement[] stack = one.getStackTrace();
        checkStack(one, stack, depth);
        // Get stack traces of all Threads
        for (Map.Entry<Thread, StackTraceElement[]> entry : Thread.getAllStackTraces().entrySet()) {
            Thread t = entry.getKey();
            stack = entry.getValue();
            if (t == null || stack == null) {
                throw new RuntimeException("Null thread or stacktrace returned");
            }
            if (t == one) {
                checkStack(t, stack, depth);
            }
        }
    }
    private static void checkStack(Thread t, StackTraceElement[] stack, int depth) throws Exception {
        if (trace) {
            printStack(t, stack);
        }
        int frame = stack.length - 1;
        for (int i = 0; i < depth && frame >= 0; i++) {
            if (! stack[frame].getMethodName().equals(methodNames[i])) {
                throw new RuntimeException("Expected " + methodNames[i] + " in frame " + frame + " but got " +
                                           stack[frame].getMethodName());
            }
            frame--;
        }
    }
    private static void printStack(Thread t, StackTraceElement[] stack) {
        System.out.println(t + " stack: (length = " + stack.length + ")");
        if (t != null) {
            for (int j = 0; j < stack.length; j++) {
                System.out.println(stack[j]);
            }
            System.out.println();
        }
    }
}
// EXEC:%maple  GenerifyStackTraces.java -p %platform %build_option -o %n.so
// EXEC:%run -b %host -l %username -p %port --run_type %run_type --sn %sn  %n.so GenerifyStackTraces  %mplsh_option| compare %f
// ASSERT: scan 0\n
