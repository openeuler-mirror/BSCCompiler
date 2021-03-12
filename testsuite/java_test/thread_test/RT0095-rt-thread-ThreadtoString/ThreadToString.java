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


public class ThreadToString extends Thread {
    public ThreadToString(ThreadGroup group, String name) {
        super(group, name);
    }
    public static void main(String[] args) {
        String string;
        ThreadGroup threadGroup = new ThreadGroup("god");
        ThreadToString threadToString = new ThreadToString(threadGroup, "good");
        threadToString.setPriority(8);
        string = threadToString.toString();
        if (string.equals("Thread[good,8,god]")) {
            System.out.println(0);
        }
    }
}