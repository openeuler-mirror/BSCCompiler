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
    // Test for setName()
    public static void main(String[] args) throws Exception {
        Thread thread = new Thread();
        String newName = "maple_thread";
        thread.setName(newName);
        System.out.println("set the new name -- " + newName + " ---- " + thread.getName());
        /**
         * Verify the setName(null) method, and throw NullPointerException
        */

        Thread t = new Thread();
        try {
            t.setName(null);
        } catch (NullPointerException e) {
            System.out.println("setName() should not accept null names");
        }
    }
}