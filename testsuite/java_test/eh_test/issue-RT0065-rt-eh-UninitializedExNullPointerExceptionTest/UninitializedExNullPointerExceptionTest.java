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
public class UninitializedExNullPointerExceptionTest {
    static int processResult = 2;
    static UninitializedExNullPointerExceptionTest iv;
    private void proc2() {}
    /**
     * Test Jvm instr: invoke special throw NullPointException.
    */

    public static void proc1() {
        try {
            iv.proc2(); // the invoke special instruction is used here
        } catch (NullPointerException e) {
            processResult = 0; // STATUS_PASSED
        }
    }
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        proc1();
        return processResult;
    }
}
