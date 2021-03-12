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


public class ThreadSetPriority5 extends Thread {
    public static void main(String[] args) {
        Integer i = null;
        ThreadSetPriority5 thread_obj1 = new ThreadSetPriority5();
        try {
            thread_obj1.setPriority(i);
            System.out.println(2);
        } catch (IllegalArgumentException e) {
            System.out.println(2);
        } catch (NullPointerException e) {
            System.out.println(0);
        }
    }
}
