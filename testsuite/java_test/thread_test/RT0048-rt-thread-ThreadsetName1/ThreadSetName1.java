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


public class ThreadSetName1 extends Thread {
    public static void main(String[] args) {
        ThreadSetName1 thread_obj = new ThreadSetName1();
        thread_obj.start();
        thread_obj.setName("我abc123!?*{$");
        if (thread_obj.getName().equals("我abc123!?*{$")) {
            System.out.println(0);
        }
    }
}
