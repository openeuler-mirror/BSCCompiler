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
interface CaseInterface {
    float FIELD1 = ClinitBase005.getCount1();
    String FIELD2 = ClinitBase005.getCount2();
    OtherCase FIELD3 = (ClinitBase005.count == 12) ? ClinitBase005.getCount3() : null;
}
class OtherCase {
}
class CaseClass implements CaseInterface {
}
public class ClinitBase005 {
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
    static OtherCase getCount3() {
        count += 1;
        return new OtherCase();
    }
    private static int run(String[] argv, PrintStream out) {
        int res = 0;
        if (CaseClass.FIELD1 != 11.0f) {
            res = 2;
            out.println("Error1: interface should not be initialized");
        }
        count++;
        if (!CaseClass.FIELD2.equals("12")) {
            res = 2;
            out.println("Error2: interface should not be initialized");
        }
        count++;
        if (CaseClass.FIELD3 == null || count != 15) {
            res = 2;
            out.println("Error3: interface not initialized");
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
