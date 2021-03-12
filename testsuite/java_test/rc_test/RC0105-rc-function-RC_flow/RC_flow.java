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


public class RC_flow {
    private Object obj;
    private byte[] b;
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
        System.out.println("ExpectResult");
    }
    public static void rc_testcase_main_wrapper() {
        TestFlow1(21);
        TestFlow2(30);
        TestFlow3(56);
    }
    static Object TestFlow1(int num) {
        Object rc = null;
        for (int i = 1; i < num; i++) {
            if ((i % 5) == 0) {
                rc = new RC_flow();
            }
        }
        return rc;
    }
    static Object TestFlow2(int num) {
        Object rc = null;
        for (int i = 1; i < num; i++) {
            if ((i % 5) == 0) {
                rc = Test2();
            }
        }
        return rc;
    }
    static void TestFlow3(int num) {
        Object rc = null;
        for (int i = 1; i < num; i++) {
            if ((i % 5) == 0) {
                rc = Test2();
            }
        }
    }
    static RC_flow Test2() {
        RC_flow obj = new RC_flow();
        return obj;
    }
}
