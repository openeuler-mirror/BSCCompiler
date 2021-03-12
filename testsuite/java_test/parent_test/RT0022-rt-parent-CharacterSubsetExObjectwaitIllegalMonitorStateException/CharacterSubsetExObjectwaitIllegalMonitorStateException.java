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


import java.lang.Thread;
public class CharacterSubsetExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    private static MySubset5 csb1 = new MySubset5("some subset");
    public static void main(String argv[]) {
        System.out.println(run());
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        // final void wait()
        try {
            result = characterSubsetExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = characterSubsetExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = characterSubsetExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 20;
        }
        if (result == 4 && CharacterSubsetExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }
//        System.out.println("result: " + result);
//        System.out.println("CharacterSubsetExObjectwaitIllegalMonitorStateException.res: " + CharacterSubsetExObjectwaitIllegalMonitorStateException.res);
        return result;
    }
    private static int characterSubsetExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor
        // final void wait()
        try {
            csb1.wait();
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int characterSubsetExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis)
        long millis = 123;
        try {
            csb1.wait(millis);
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int characterSubsetExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        long millis = 123;
        int nanos = 10;
        try {
            csb1.wait(millis, nanos);
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            CharacterSubsetExObjectwaitIllegalMonitorStateException.res = CharacterSubsetExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
}
class MySubset5 extends Character.Subset {
    MySubset5(String name) {
        super(name);
    }
}