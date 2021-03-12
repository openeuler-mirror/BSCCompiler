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


public class GetStackTraceIterating {
    static final int limit = 100;
    public int rec(int i) throws Exception {
        if (i == limit) {
            throw new Exception();
        } else {
            return rec(i + 1);
        }
    }
    public void run(int reps) {
        int sum = 0;
        for (int i = 0; i < reps; i++) {
            try {
                sum += rec(1);
            } catch (Exception e) {
                boolean unexpected = false;
                StackTraceElement[] stackTrace = e.getStackTrace();
                for (StackTraceElement trace : stackTrace) {
                    String message = trace.getFileName();
                    if (message.contains("GetStackTraceIterating")) {
                        unexpected = false;
                    } else {
                        unexpected = true;
                    }
                    if (unexpected) {
                        System.out.println(message);
                    }
                }
            }
        }
    }
    public static void main(String[] args) {
        GetStackTraceIterating obj = new GetStackTraceIterating();
        obj.run(100);
        System.out.println(0);
    }
}
// EXEC:%maple  GetStackTraceIterating.java -p %platform %build_option -o %n.so
// EXEC:%run -b %host -l %username -p %port --run_type %run_type --sn %sn  %n.so GetStackTraceIterating  %mplsh_option | compare %f
// ASSERT: scan 0\n
