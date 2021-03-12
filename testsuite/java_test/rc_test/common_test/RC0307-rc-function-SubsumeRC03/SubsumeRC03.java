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


public class SubsumeRC03 {
    private String str;
    public SubsumeRC03() {
    }
    public void testfunc(SubsumeRC03 t) {
        str = t.str;
        return ;
    }
    public static void main(String[] args){
        SubsumeRC03 temp0 = new SubsumeRC03();
        SubsumeRC03 temp1 = new SubsumeRC03();
        temp1.testfunc(temp0);
        System.out.println("ExpectResult");
    }
}
