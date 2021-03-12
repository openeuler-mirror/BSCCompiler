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
class FatherClass {
    static int field;
    static {
        if (ClinitSeq001.getCount() == 1) {
            field = ClinitSeq001.getCount();
        }
    }
}
class ChildClass extends FatherClass {
    static int field;
    static {
        if (ClinitSeq001.getCount() == 3) {
            field = ClinitSeq001.getCount();
        }
    }
}
public class ClinitSeq001 {
    static private int count = 0;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out) /*STATUS_TEMP*/);
    }
    static int getCount() {
        count += 1;
        return count;
    }
    private static int run(String[] argv, PrintStream out) {
        int res = 0/*STATUS_PASSED*/;
        if (ChildClass.field != 4) {
            res = 2;
            out.println("Error, ChildClass not initialized");
        }
        if (FatherClass.field != 2) {
            res = 2;
            out.println("Error, FatherClass not initialized");
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
