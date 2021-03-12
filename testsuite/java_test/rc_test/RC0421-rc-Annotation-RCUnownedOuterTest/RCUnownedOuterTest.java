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


import com.huawei.ark.annotation.UnownedOuter;
import java.util.LinkedList;
import java.util.List;
public class RCUnownedOuterTest {
    public static void main(String[] args) {
        UnownedAnnoymous unownedAnnoymous = new UnownedAnnoymous();
        unownedAnnoymous.anonymousCapture();
        UnownedInner unownedInner = new UnownedInner();
        unownedInner.method();
        UnownedInner.InnerClass innerClass = unownedInner.new InnerClass();
        innerClass.myName();
        if (unownedAnnoymous.checkAnnoy == 1 && unownedInner.checkInner == 1) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("unownedAnnoymous.checkAnnoy:" + unownedAnnoymous.checkAnnoy);
            System.out.println("unownedInner.checkInner:" + unownedInner.checkInner);
        }
    }
}
class UnownedInner {
    String name = "test";
    int checkInner = 0;
    InnerClass innerClass;
    void method() {
        innerClass = new InnerClass();
    }
    @UnownedOuter
    class InnerClass {
        void myName() {
            checkInner = 0;
            String myname;
            myname = name + name;
            if (myname.equals("testtest")) {
                checkInner++;
            }
        }
    }
}
class UnownedAnnoymous {
    String name = "test";
    static int checkAnnoy;
    void anonymousCapture() {
        Runnable r = new Runnable() {
            @UnownedOuter
            @Override
            public void run() {
                checkAnnoy = 0;
                String myName = name + name;
                if (myName.equals("testtest")) {
                    checkAnnoy++;
                }
            }
        };
        r.run();
    }
}
