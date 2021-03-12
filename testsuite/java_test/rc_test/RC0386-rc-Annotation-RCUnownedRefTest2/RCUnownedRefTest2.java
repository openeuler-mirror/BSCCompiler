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


import com.huawei.ark.annotation.Unowned;
class Test_B {
    @Unowned
    Test_B bself;
    protected void run() {
        System.out.println("ExpectResult");
    }
}
class Test_A {
    Test_B bb;
    Test_B bb2;
    public void test() {
        foo();
        bb.bself.run();
        bb2.bself.run();
    }
    private void foo() {
        bb = new Test_B();
        bb2 = new Test_B();
        bb.bself = bb;
        bb2.bself = bb;
    }
}
public class RCUnownedRefTest2 {
    public static void main(String[] args) {
        new Test_A().test();
    }
}
