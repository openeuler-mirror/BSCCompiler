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
public class TryCatchResourceExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 4; /* STATUS_FAILED*/

        try {
            // No Exception occur: conn.start->conn.close->finally.
            result = result - tryCatchResourceExceptionTest1();
            // Exception in try: conn.start->conn.close->catch(catch e in try,ignore e in close)->finally.
            result = result - tryCatchResourceExceptionTest2();
            // Exception in try: conn.start->conn.close->catch(catch e in start,ignore e in try & close)->finally.
            result = result - tryCatchResourceExceptionTest3();
        } catch (Exception e) {
            processResult += 10;
        }
        if (result == 1 && processResult == 90) {
            result = 0;
        }
        return result;
    }
    /**
     * No Exception occur: conn.start->conn.close->finally.
     * @return status code
    */

    public static int tryCatchResourceExceptionTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        try (MyResourceA conn = new MyResourceA()) {
            conn.start();
            result1 = 1;
            processResult--;
        } catch (Exception e) {
            System.out.println("catch ......");
            processResult -= 10;
            result1++;
        } finally {
            processResult--;
        }
        processResult--;
        return result1;
    }
    /**
     * Exception in try: conn.start->conn.close->catch(catch e in try,ignore e in close)->finally.
     * @return status code
    */

    public static int tryCatchResourceExceptionTest2() {
        int result1 = 4; /* STATUS_FAILED*/

        try (MyResourceB conn = new MyResourceB()) {
            conn.start();
            int num = 1 / (result1 - 4);
            processResult -= 10;
        } catch (Exception e) {
            processResult--;
            result1 = 1;
        } finally {
            processResult--;
        }
        processResult--;
        return result1;
    }
    /**
     * Exception in try: conn.start->conn.close->catch(catch e in start,ignore e in try & close)->finally.
     * @return status code
    */

    public static int tryCatchResourceExceptionTest3() {
        int result1 = 4; /* STATUS_FAILED*/

        try (MyResourceC conn = new MyResourceC()) {
            conn.start();
            int num = 1 / (result1 - 4);
            processResult -= 10;
        } catch (Exception e) {
            processResult--;
            result1 = 1;
        } finally {
            processResult--;
        }
        processResult--;
        return result1;
    }
    public static class MyResourceA implements AutoCloseable {
        /**
         * start fun
        */

        public void start() {
            processResult = processResult * 2;
        }
        @Override
        public void close() {
            processResult = processResult / 2;
        }
    }
    public static class MyResourceB implements AutoCloseable {
        /**
         * start fun
        */

        public void start() {
            processResult = processResult * 2;
        }
        @Override
        public void close() throws Exception {
            processResult = processResult / 2;
            throw new NoSuchFieldException("ee");
        }
    }
    public static class MyResourceC implements AutoCloseable {
        /**
         * start fun
        */

        public void start() throws Exception {
            processResult = processResult * 2;
            throw new NoSuchMethodException("mm");
        }
        @Override
        public void close() throws Exception {
            processResult = processResult / 2;
            throw new NoSuchFieldException("ee");
        }
    }
}
