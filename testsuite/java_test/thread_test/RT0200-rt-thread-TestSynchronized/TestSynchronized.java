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
class Offset {
    int[] offsets = null;
    public void setOffsets(int[] newOne) {
        offsets = newOne;
    }
    public void clear() {
        offsets = null;
    }
}
public class TestSynchronized {
    public static Object obj = new Object();
    public static Offset getOffset() {
        int[] off = {-4, -5, -6, -7};
        Offset ret = new Offset();
        ret.setOffsets(off);
        if (ret.offsets.length < 8) {
            ret = null;
        }
        return ret;
    }
    public static int[] getCurLength() {
        synchronized (obj) {
            int[] off1 = {-1, -1, -1, -1};
            Offset o = getOffset();
            if (off1.length > 5) {
                int[] off2 = {9, 9, 9};
                if (off2.length == o.offsets.length) {
                    for (int i = 0; i < o.offsets.length; i++) {
                        o.offsets[i] = off2[i];
                    }
                }
            }
            System.arraycopy(o.offsets, 0, off1, 0, o.offsets.length);
            return off1;
        }
    }
    public static void main(String args[]) {
        try {
            getCurLength();
            Thread.sleep(2000);
        } catch (NullPointerException e) {
            System.out.println("Pass");
        } catch (InterruptedException e1) {}
        Thread t = new Thread(new Runnable() {
            public void run() {
                synchronized (obj) {
                    System.out.println("Pass");
                }
            }
        });
        t.setDaemon(true);
        t.start();
    }
}
