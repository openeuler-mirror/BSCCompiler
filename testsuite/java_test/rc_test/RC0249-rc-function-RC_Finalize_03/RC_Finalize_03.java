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
class RC_Finalize_031 {
    public void finalize() {
        RC_Finalize_03.fo_finalized = true;
        RC_Finalize_03.dummy = 1 / RC_Finalize_03.zero;
        RC_Finalize_03.fo_exception_occerred = false;
    }
}
public class RC_Finalize_03 {
    public static boolean fo_finalized;
    public static boolean fo_exception_occerred;
    public static final long TIMEOUT = 50000;
    public static final int zero = 0;
    public static int dummy;
    public static int run(String argv[], PrintStream out) {
        fo_finalized = false;
        fo_exception_occerred = true;
        RC_Finalize_031 cl1 = new RC_Finalize_031();
        cl1 = null;
        long startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < TIMEOUT){
            Runtime.getRuntime().gc();
            Runtime.getRuntime().runFinalization();
            if (!fo_finalized) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    //out.println("InterruptedException: " + e);
                    return 2;
                } catch(Throwable e) {
                    //out.println("Throwable: " + e);
                    return 2;
                }
            } else {
                break;
            }
        }
        if (!fo_finalized) {
            //out.println("Ok, RC_Finalize_031 was not finalized during " + TIMEOUT/1000 + "sec");
            return 0;
        }
        if (fo_exception_occerred) {
            return 0;
        } else {
            //out.println("Failed: expected exception is not thrown");
        }
        return 2;
    }
    public static void main(String argv[]) {
        if(run(argv, System.out)==0)
			System.out.println("ExpectResult");
    }
} // end RC_Finalize_03
