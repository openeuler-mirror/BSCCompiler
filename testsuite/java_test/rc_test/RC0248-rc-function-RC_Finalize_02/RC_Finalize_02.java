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
public class RC_Finalize_02 {
    public void finalize() { 
	System.out.println("ExpectResult");
    }
    public static int run(String argv[], PrintStream out) {
	return 0;
    }
    public static void main(String argv[]) {
	System.runFinalizersOnExit(true);
	RC_Finalize_02 testClass = new RC_Finalize_02();
    }
} // end RC_Finalize_02
