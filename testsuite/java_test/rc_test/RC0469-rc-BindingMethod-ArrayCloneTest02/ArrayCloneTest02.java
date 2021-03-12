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


public class ArrayCloneTest02 {
    private Integer id;
    public ArrayCloneTest02(Integer id) {
        this.id = id;
    }
    public Integer getId() {
        return id;
    }
    public void setId(Integer id) {
        this.id = id;
    }
    @Override
    public String toString() {
        return "CloneTest{" +
                "id=" + id +
                '}';
    }
    public static void main(String[] args) {
        try {
            ArrayCloneTest02 cloneTest = new ArrayCloneTest02(1);
            ArrayCloneTest02 cloneTest1 = (ArrayCloneTest02) cloneTest.clone();
            System.out.println("ErrorResult");
        } catch (CloneNotSupportedException e) {
            System.out.println("ExpectResult");
        }
    }
}
