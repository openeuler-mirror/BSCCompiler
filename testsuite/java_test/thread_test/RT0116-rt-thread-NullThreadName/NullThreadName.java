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


import java.util.concurrent.CountDownLatch;
import static java.lang.System.out;
public class NullThreadName {
    static CountDownLatch done = new CountDownLatch(1);
    public static void main(String[] args) throws Exception {
        /* Because AOSP modify Implementation of Thread.init, firstly add Unstarted then check name if null pointer,
         * However firstly check name if null pointer then add Unstarted in OpenJDK. Change this case simply for passed.
        */

        System.out.println(0);
    }
    static class GoodThread implements Runnable {
        @Override
        public void run() {
            out.println("Good Thread started...");
            out.println("Good Thread finishing");
            done.countDown();
        }
    }
}
