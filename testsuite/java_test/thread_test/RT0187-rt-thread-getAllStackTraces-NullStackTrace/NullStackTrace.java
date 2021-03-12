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


import java.util.Map;
public class NullStackTrace extends Thread {
    static final int TIMES = 500;
    public static void main(String[] args) {
        for (int i = 0; i < TIMES; i++) {
            Thread t = new Thread();
            t.start();
            Map m = getAllStackTraces();
            String stFinish = m.keySet().toString();
            if (stFinish == null)
                throw new RuntimeException("Failed: Thread.getAllStackTrace should not return null");
        }
        System.out.println("0");
    }
}
// EXEC:%maple  NullStackTrace.java -p %platform %build_option -o %n.so
// EXEC:%run -b %host -l %username -p %port --run_type %run_type --sn %sn  %n.so NullStackTrace  %mplsh_option | compare %f
// ASSERT: scan 0\n
