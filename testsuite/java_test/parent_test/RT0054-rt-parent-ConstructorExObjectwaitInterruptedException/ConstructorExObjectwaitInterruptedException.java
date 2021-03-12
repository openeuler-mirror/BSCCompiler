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


import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Constructor;
public class ConstructorExObjectwaitInterruptedException {
    static int res = 99;
    private static Constructor<?> con = null;
    public static void main(String argv[]) throws NoSuchMethodException, SecurityException {
        con = SampleClass18.class.getConstructor(String.class);
        System.out.println(new ConstructorExObjectwaitInterruptedException().run());
    }
    private class ConstructorExObjectwaitInterruptedException11 implements Runnable {
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (con) {
                con.notifyAll();
                try {
                    con.wait();
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 10;
                }
            }
        }
    }
    private class ConstructorExObjectwaitInterruptedException21 implements Runnable {
        // final void wait(long millis)
        long millis = 10000;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (con) {
                con.notifyAll();
                try {
                    con.wait(millis);
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 5;
                }
            }
        }
    }
    private class ConstructorExObjectwaitInterruptedException31 implements Runnable {
        // final void wait(long millis, int nanos)
        long millis = 10000;
        int nanos = 100;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (con) {
                con.notifyAll();
                try {
                    con.wait(millis, nanos);
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ConstructorExObjectwaitInterruptedException.res = ConstructorExObjectwaitInterruptedException.res - 5;
                }
            }
        }
    }
    /**
     * sleep fun
     *
     * @param slpnum wait time
    */

    public void sleep(int slpnum) {
        try {
            Thread.sleep(slpnum);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        // check api normal
        // final void wait()
        Thread t1 = new Thread(new ConstructorExObjectwaitInterruptedException11());
        // final void wait(long millis)
        Thread t3 = new Thread(new ConstructorExObjectwaitInterruptedException21());
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ConstructorExObjectwaitInterruptedException31());
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t3.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t5.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t1.start();
        sleep(1000);
        t1.interrupt();
        sleep(1000);
        t3.start();
        sleep(1000);
        t3.interrupt();
        sleep(1000);
        t5.start();
        sleep(1000);
        t5.interrupt();
        sleep(1000);
        if (result == 2 && ConstructorExObjectwaitInterruptedException.res == 96) {
            result = 0;
        }
        return result;
    }
}
@CustomAnnotations18(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass18 {
    private String sampleField;
    @CustomAnnotations18(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass18(String str) {
    }
    public String getSampleField() {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE, ElementType.METHOD, ElementType.CONSTRUCTOR})
@interface CustomAnnotations18 {
    String name();
    String value();
}
