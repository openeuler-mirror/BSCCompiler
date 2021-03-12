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
    public static void main(String[] args) throws Exception {
        Thread thread1 = new Thread();
        Thread thread2 = new Thread();
        long tId1 = thread1.getId();
        long tId2 = thread2.getId();
        System.out.println(tId1 != tId2);
        thread1.join();
        thread2.join();
        long tId3 = thread1.getId();
        long tId4 = thread2.getId();
        System.out.println(tId1 == tId3);
        System.out.println(tId2 == tId4);
    }
}