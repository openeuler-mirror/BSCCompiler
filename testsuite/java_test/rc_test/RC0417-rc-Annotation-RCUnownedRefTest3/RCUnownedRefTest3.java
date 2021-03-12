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
import java.util.LinkedList;
import java.util.List;
public class RCUnownedRefTest3 {
    public static void main(String[] args) {
        UnownedAnnoymous unownedAnnoymous = new UnownedAnnoymous();
        unownedAnnoymous.anonymousCapture("test");
        UnownedLambda unownedLambda = new UnownedLambda();
        List<String> list = new LinkedList<>();
        list.add("test");
        unownedLambda.lambdaCapture(list, "test");
        if (unownedAnnoymous.checkAnnoy == true && unownedLambda.checkLambda == true) {
            System.out.println("ExpectResult");
        }
    }
}
class UnownedLambda {
    String name;
    static boolean checkLambda = false;
    void lambdaCapture(List<String> list, String suffix) {
        final @Unowned String capSuffix = suffix;
        final @Unowned UnownedLambda self = this;
        list.forEach((str) -> {
            if ((self.name + str + capSuffix).equals("nulltesttest")) {
                checkLambda = true;
            }
        });
    }
}
class UnownedAnnoymous {
    String name;
    static boolean checkAnnoy = false;
    void anonymousCapture(String suffix) {
        final @Unowned String capSuffix = suffix;
        Runnable r = new Runnable() {
            @Override
            public void run() {
                if ((name + capSuffix).equals("nulltest")) {
                    checkAnnoy = true;
                }
            }
        };
        r.run();
    }
}
