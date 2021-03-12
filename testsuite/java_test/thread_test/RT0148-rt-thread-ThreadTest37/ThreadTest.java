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


public class ThreadTest {
    public static void main(String[] args) throws Exception {
        long millis = 2000;
        try {
            Thread.sleep(millis);
            System.out.println("sleep(long millis) OK");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException for sleep(long millis) as: millis < 0");
        }
        try {
            Thread.sleep(-2);
            System.out.println("sleep(long millis) OK");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException for sleep(long millis) as: millis < 0");
        }
        int nanos = 123456;
        try {
            Thread.sleep(millis, nanos);
            System.out.println("sleep(long millis, int nanos) OK");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException for sleep(long millis, int nanos) as: millis < 0 or nanos < 0 or nanos > 999999");
        }
        try {
            Thread.sleep(millis, -1);
            System.out.println("sleep(long millis, int nanos) OK");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException for sleep(long millis, int nanos) as: nanos < 0");
        }
        try {
            Thread.sleep(millis, 1123456);
            System.out.println("sleep(long millis, int nanos) OK");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException for sleep(long millis, int nanos) as: nanos > 999999");
        }
        try {
            Thread.sleep(-3, nanos);
            System.out.println("sleep(long millis, int nanos) OK");
        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException for sleep(long millis, int nanos) as: millis < 0");
        }
    }
}