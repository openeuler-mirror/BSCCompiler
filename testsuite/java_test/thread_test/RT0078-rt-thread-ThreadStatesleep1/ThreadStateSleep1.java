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


public class ThreadStateSleep1 extends Thread {
    public static void main(String[] args) {
        ThreadStateSleep1 threadStateSleep1 = new ThreadStateSleep1();
        ThreadStateSleep1 threadStateSleep2 = new ThreadStateSleep1();
        ThreadStateSleep1 threadStateSleep3 = new ThreadStateSleep1();
        ThreadStateSleep1 threadStateSleep4 = new ThreadStateSleep1();
        threadStateSleep1.start();
        threadStateSleep2.start();
        try {
            sleep(1000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        threadStateSleep3.start();
        threadStateSleep4.start();
        if (threadStateSleep1.getState().toString().equals("TERMINATED")
                && threadStateSleep4.getState().toString().equals("RUNNABLE")) {
            System.out.println(0);
            return;
        }
        if (threadStateSleep1.getState().toString().equals("TERMINATED")
                && threadStateSleep4.getState().toString().equals("TERMINATED")) {
            System.out.println(0);
        }
    }
}