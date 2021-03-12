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


import java.io.PrintStream;
class Other {
}
class CaseClass {
    static final float FIELD1 = ClinitBase004.getCount1();
    static final String FIELD2 = ClinitBase004.getCount2();
    static Other field3;
    static {
        if (ClinitBase004.count == 12) {
            field3 = ClinitBase004.getCount3();
        }
    }
}
public class ClinitBase004 {
    public static int count;
    static {
        count = 10;
    }
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    static float getCount1() {
        count += 1;
        return (float) count;
    }
    static String getCount2() {
        count += 1;
        return String.valueOf(count);
    }
    static Other getCount3() {
        count += 1;
        return new Other();
    }
    private static int run(String[] argv, PrintStream out) {
        int res = 0;
        // 访问静态常量不应该触发类初始化
        if (CaseClass.FIELD1 != 11.0f) {
            res = 2;
            out.println("Error1: CaseClass should not be initialized");
        }
        count++;
        if (!CaseClass.FIELD2.equals("12")) {
            res = 2;
            out.println("Error2: CaseClass should not be initialized");
        }
        count++;
        // 访问静态变量触发类初始化
        if (CaseClass.field3 == null || count != 15) {
            res = 2;
            out.println("Error3: CaseClass not initialized");
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
