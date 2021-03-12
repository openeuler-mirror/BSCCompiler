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

class MyCycle {
    byte[] bytes;
    Object[] refs;
    MyCycle() {
        bytes = new byte[2500];
        refs = new Object[1];
        refs[0] = this;
    }
};
public class TestJniPassingFloat {
    static {
        System.loadLibrary("jniFunction");
    }
    static volatile boolean running = true;
    static native float myNative(float f, double d);
    static void run() {
        MyCycle obj;
        for (int i = 0; i < 30000; ++i) {
            obj = new MyCycle();
        }
        running = false;
    }
    public static void main(String[] args) throws Exception {
        Thread thread = new Thread(() -> run());
        thread.start();
        while (running) {
            float f = myNative(1.0f, 2.0f);
            if (Float.compare(f, 1.0f + 2.0f) != 0) {
                throw new Exception("test failed f=" + f);
            }
        }
        thread.join();
        System.out.println("pass");
    }
}
