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


public class Daemon {
    public static void main(String[] args) throws Exception {
        ThreadGroup tg = new ThreadGroup("madbot-threads");
        Thread myThread = new MadThread(tg, "mad");
        ThreadGroup aGroup = new ThreadGroup(tg, "ness");
        tg.setDaemon(true);
        if (tg.activeCount() != 0)
            throw new RuntimeException("activeCount");
        aGroup.destroy();
        if (tg.isDestroyed())
            throw new RuntimeException("destroy");
        try {
            Thread anotherThread = new MadThread(aGroup, "bot");
            throw new RuntimeException("illegal");
        } catch (IllegalThreadStateException itse) {
            // Correct result
        }
        System.out.println("Passed");
    }
}
class MadThread extends Thread {
    String name;
    MadThread(ThreadGroup tg, String name) {
        super(tg, name);
        this.name = name;
    }
    public void run() {
        System.out.println("me run " + name);
    }
}