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

import java.lang.annotation.*;
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@AnnoA( value = @AnnoB(intB = -1))
@AnnoA( value = {@AnnoB(intB = -2), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -3), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -4), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -5), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -6), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -7), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -8), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -9), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -10), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
public @interface Test{
    AnnoA[] value();
}
