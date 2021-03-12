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
import java.io.PrintWriter;
import java.io.StringWriter;
public class TryResourceStartAndCloseExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = tryResourceStartAndCloseExceptionTest1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 1 && processResult == 94) {
            result = 0;
        }
        return result;
    }
    public static class MyResource implements AutoCloseable {
        /**
         * start fun
         * @throws NumberFormatException
        */

        public void start() throws NumberFormatException {
            if (processResult == 99) {
                processResult--;
            }
            throw new NumberFormatException("start:exception!!!");
        }
        @Override
        public void close() throws StringIndexOutOfBoundsException {
            if (processResult == 98) {
                processResult--;
            }
            throw new StringIndexOutOfBoundsException("end:exception!!!");
        }
    }
    /**
     * Test try(exception1){}-catch(e1){}-close(exception2)-finally.
     * @return status code
    */

    public static int tryResourceStartAndCloseExceptionTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        try (MyResource conn = new MyResource()) {
            conn.start();
        } catch (NumberFormatException e) {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            e.printStackTrace(pw);
            String msg=sw.toString();
//            System.out.println(msg);
            if (processResult == 97 && msg.contains("start:exception!!!") && msg.contains("end:exception!!!")) {
                processResult--;
            }
            result1 = 1;
        } catch (StringIndexOutOfBoundsException e) {
            processResult -= 10;
            result1 = 3;
        } finally {
            if (processResult == 96) {
                processResult--;
            }
        }
        if (processResult == 95) {
            processResult--;
        }
        return result1;
    }
}
