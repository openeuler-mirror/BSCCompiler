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
interface JvmInterfaceUnsatisfiedLinkErrorTesti {
    /**
     * just a simple method for test
     * @return null
    */

    int testFail();
}
class JvmInterfaceUnsatisfiedLinkErrorTesta implements JvmInterfaceUnsatisfiedLinkErrorTesti {
    /**
     * just a simple native method for test
     * @return null
    */

    public native int testFail();
}
public class JvmInterfaceUnsatisfiedLinkErrorTest {
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        JvmInterfaceUnsatisfiedLinkErrorTesta test = new JvmInterfaceUnsatisfiedLinkErrorTesta();
        try {
            int tmp = test.testFail();
        } catch (UnsatisfiedLinkError e1) {
            return 0; // STATUS_PASSED
        }
        return 2; // STATUS_FAILED
    }
}
