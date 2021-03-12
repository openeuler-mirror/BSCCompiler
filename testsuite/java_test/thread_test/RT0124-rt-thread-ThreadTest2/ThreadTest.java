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


public class ThreadTest {
    /**
     * Get active threads count; should be > 0
    */

    public static void main(String[] args) throws Exception {
        int activeCount = Thread.activeCount();
        System.out.println("The active threads count == " + activeCount);
        Thread thread = new Thread(() -> {
            try {
                Thread.sleep(1000);
            } catch (Exception e) {
            }
        });
        thread.start();
        try {
            Thread.sleep(500);
        } catch (InterruptedException e1) {
        }
        activeCount = Thread.activeCount();
        System.out.println("The active threads count == " + activeCount);
    }
}