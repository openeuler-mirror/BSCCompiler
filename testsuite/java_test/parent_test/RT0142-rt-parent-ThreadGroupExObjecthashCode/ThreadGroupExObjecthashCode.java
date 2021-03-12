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


import java.lang.ThreadGroup;
public class ThreadGroupExObjecthashCode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new ThreadGroupExObjecthashCode().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = threadGroupExObjecthashCode1();
        } catch (Exception e) {
            ThreadGroupExObjecthashCode.res = ThreadGroupExObjecthashCode.res - 20;
        }
        if (result == 4 && ThreadGroupExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private int threadGroupExObjecthashCode1() {
        int result1 = 4; /*STATUS_FAILED*/
        // int hashCode()
        ThreadGroup gr1 = new ThreadGroup("Thread8023");
        ThreadGroup gr2 = gr1;
        ThreadGroup gr3 = new ThreadGroup("Thread1988");
        if (gr1.hashCode() == gr2.hashCode() && gr1.hashCode() != gr3.hashCode()) {
            ThreadGroupExObjecthashCode.res = ThreadGroupExObjecthashCode.res - 10;
        } else {
            ThreadGroupExObjecthashCode.res = ThreadGroupExObjecthashCode.res - 5;
        }
        return result1;
    }
}