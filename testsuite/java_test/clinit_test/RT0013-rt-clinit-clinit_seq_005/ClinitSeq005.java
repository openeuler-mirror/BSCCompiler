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
    int FIELD = ClinitSeq005.getCount();
    CaseOtherClass MEMBER = new CaseOtherClass();
    int uselessFunction(int a, int b);
}
class CaseOtherClass {
    static int field;
    static {
        if (ClinitSeq005.getCount() == 4) {
            field = ClinitSeq005.getCount();
        }
    }
}
class CaseChild implements CaseInterface {
    static int field;
    static {
        if (ClinitSeq005.getCount() == 1) {
            field = ClinitSeq005.getCount();
        }
    }
    public int uselessFunction(int a, int b) {
        return a + b;
    }
}
public class ClinitSeq005 {
    static private int count = 0;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    static int getCount() {
        count += 1;
        return count;
    }
    private static int run(String[] argv, PrintStream out) {
        int res = 0;
        if (CaseChild.field != 2) {
            res = 2;
            out.println("Error, child not initialized");
        }
        if (CaseInterface.MEMBER.field != 5) {
            res = 2;
            out.println("Error, other not initialized");
        }
        if (CaseInterface.FIELD != 3) {
            res = 2;
            out.println("Error, interface not initialized");
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
