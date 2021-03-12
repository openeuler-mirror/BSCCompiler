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


public class StmtTest10 {
    public static void main(String[] argv) {
        int result = 2;
        boolean check;
        String str = "123#";  // 1
        try {
            str = "123456"; // 2
            try {
                Integer.parseInt(str);
            } catch (NumberFormatException e) {
                str = "123#456";  // 3
                result--;
            } catch (NullPointerException e) {
                str = "123456#"; // 4
                result = 2;
            } catch (OutOfMemoryError e) {
                str += "123#456";  // 与32行被其他优化了
                result = 2;
            } finally {
                str = "123#456";
            }
            Integer.parseInt(str);
        } catch (NumberFormatException e) {
            str = "123#456";
            result--;
        } catch (NullPointerException e) {
            str = "123456";
            result = 2;
        } catch (OutOfMemoryError e) {
            str += "123#456";
            result = 2;
        } finally {
            check = str == "123#456";
            result--;
        }
        if (check == true && result == 0) {
            System.out.println("123#");
        }
    }
}
