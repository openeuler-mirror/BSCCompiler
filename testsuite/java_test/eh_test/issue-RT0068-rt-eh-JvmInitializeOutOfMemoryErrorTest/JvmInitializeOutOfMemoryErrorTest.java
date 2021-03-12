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
public class JvmInitializeOutOfMemoryErrorTest {
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        JvmInitializeOutOfMemoryErrorTest tTest = new JvmInitializeOutOfMemoryErrorTest();
        return tTest.test();
    }
    /**
     * Test jvm instr: invoke virtual thrown Exception InInitializerError.
     * @return status code
    */

    public static int test() {
        try {
            int[] num;
            num = new int[1024 * 1024 * 1280]; // 1024*1024*4B = 4M, *128=512M
            num[0] = 1; // If no this, y = new int[] will be optimizated and deleted.
        } catch (OutOfMemoryError e2) {
            return 0;
        }
        return 2;
    }
}
