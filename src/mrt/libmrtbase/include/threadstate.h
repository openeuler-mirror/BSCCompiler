/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef MAPLE_RUNTIME_THREAD_STATE_H
#define MAPLE_RUNTIME_THREAD_STATE_H

enum ThreadState {
  //                               Thread.State   JDWP state
  kStarting,                    // NEW            TS_WAIT      native thread started, not yet ready to run managed code
  kRunnable,                    // RUNNABLE       TS_RUNNING   runnable
  kBlocked,                     // BLOCKED        TS_MONITOR   blocked on a monitor
  kWaiting,                     // WAITING        TS_WAIT      in Object.wait()
  kTimedWaiting,                // TIMED_WAITING  TS_WAIT      in Object.wait() with a timeout
  kTerminated,                  // TERMINATED     TS_ZOMBIE    Thread.run has returned, but Thread* still around
};

#endif // MAPLE_RUNTIME_THREAD_STATE_H
