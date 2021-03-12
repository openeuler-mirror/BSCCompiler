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
class JvmStaticExceptionInInitializerErrorTesta {
    static int other_class_test_field = 21;
    static int local_field = 5 / 0;
    static int testFail() {
        int aNum = 1;
        return aNum;
    }
}
public class JvmStaticExceptionInInitializerErrorTest {
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        try {
            int tmp = JvmStaticExceptionInInitializerErrorTesta.testFail();
        } catch (ExceptionInInitializerError e1) {
            try {
                int tmp = JvmStaticExceptionInInitializerErrorTesta.testFail();
            } catch (NoClassDefFoundError e2) {
                return 0; // STATUS_PASSED
            }
        }
        return 2; // STATUS_FAILED
    }
}
