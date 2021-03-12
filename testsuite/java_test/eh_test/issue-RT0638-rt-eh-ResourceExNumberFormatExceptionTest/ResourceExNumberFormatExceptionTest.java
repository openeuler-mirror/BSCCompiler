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


import java.io.*;
public class ResourceExNumberFormatExceptionTest {
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
            result = resourceExNumberFormatExceptionTest();
        } catch (IOException e) {
            e.printStackTrace();
        }
        if (result == 1 && processResult == 95) {
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
        public void close() {}
    }
    /**
     * Test 1 try with resource.
     * @return status code
    */

    public static int resourceExNumberFormatExceptionTest() throws IOException {
        int result1 = 4; /* STATUS_FAILED*/

        String exception = "";
        try (MyResource conn = new MyResource()) {
            conn.start();
        } catch (NumberFormatException e) {
            //            e.printStackTrace();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            e.printStackTrace(new PrintStream(baos)); // 将e的调用栈信息输出到ByteArrayOutputStream
            InputStream ins = new ByteArrayInputStream(baos.toByteArray()); // 将信息放到InputStream
            BufferedReader buffer = new BufferedReader(new InputStreamReader(ins, "UTF-8"));
            int checkFlag = 0;
            String line = null;
            while ((line = buffer.readLine()) != null) {
//                System.out.println(line);
                if (line.contains("java.lang.NumberFormatException")) {
                    //                    System.out.println(checkFlag);
                    checkFlag++;
                }
                if (line.contains("ResourceExNumberFormatExceptionTest.java") && line.contains("60")) {
                    if (checkFlag == 1) {
                        //                        System.out.println(checkFlag);
                        checkFlag++;
                    }
                }
                if (line.contains("ResourceExNumberFormatExceptionTest.java") && line.contains("75")) {
                    if (checkFlag == 2) {
                        //                        System.out.println(checkFlag);
                        checkFlag++;
                    }
                }
                if (line.contains("ResourceExNumberFormatExceptionTest.java") && line.contains("40")) {
                    if (checkFlag == 3) {
                        checkFlag++;
                    }
                }
                if (line.contains("ResourceExNumberFormatExceptionTest.java") && line.contains("30")) {
                    if (checkFlag == 4) {
                        checkFlag++;
                    }
                }
            }
            if (processResult == 98) {
                if (checkFlag == 5) {
                    processResult--;
                }
            }
            result1 = 1;
        } finally {
            if (processResult == 97) {
                processResult--;
            }
        }
        if (processResult == 96) {
            processResult--;
        }
        return result1;
    }
}
