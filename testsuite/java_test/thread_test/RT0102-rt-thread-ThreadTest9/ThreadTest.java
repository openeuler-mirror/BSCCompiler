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
     * Verify currentThread() and toString()
    */

    public static void main(String[] args) throws Exception {
        System.out.println(Thread.currentThread().toString());
        System.out.println(Thread.currentThread());
        System.out.println(Thread.currentThread().getName());
        Thread thread = new Thread();
        String info = thread.toString();
        String name = thread.getName();
        System.out.println("thread's name is -- " + name + " -- " + info.indexOf(name));
        String stringPriority = new Integer(thread.getPriority()).toString();
        System.out.println("thread's priority is -- " + stringPriority + " -- " + info.indexOf(","
                + stringPriority + ","));
        String groupName = thread.getThreadGroup().getName();
        System.out.println("thread's group is -- " + groupName + " -- " + info.indexOf(groupName));
        Thread thread2 = new Thread();
        System.out.println("thread name -- " +
                thread2.toString());
        System.out.println("thread group --- " + Thread.currentThread().getThreadGroup() +
                " --- " + thread2.getThreadGroup());
    }
}