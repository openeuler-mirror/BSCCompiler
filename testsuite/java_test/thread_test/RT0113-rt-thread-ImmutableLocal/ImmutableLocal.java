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


public class ImmutableLocal {
    private static final ThreadLocal cache = new ImmutableThreadLocal() {
        public Object initialValue() {
            return Thread.currentThread().getName();
        }
    };
    public static void main(final String[] args) {
        if (cache.get().equals("main")) {
            System.out.println(0);
        }
    }
    /**
     * {@link ThreadLocal} guaranteed to always return the same reference.
    */

    abstract public static class ImmutableThreadLocal extends ThreadLocal {
        public void set(final Object value) {
            throw new RuntimeException("ImmutableThreadLocal set called");
        }
        // Force override
        abstract protected Object initialValue();
    }
}
