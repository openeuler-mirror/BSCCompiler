#!/usr/bin/env python
# coding=utf-8
#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

import os
import locale
import subprocess
import signal

from case.component import Component


class Shell(Component):

    def __init__(self, command, workdir, timeout):
        self.command = command
        self.workdir = workdir
        self.timeout = timeout
        self.return_code = None
        self.com_out = None
        self.com_err = None

    def execute(self):
        process_command = subprocess.Popen(self.command, shell=True, cwd=self.workdir, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            self.com_out, self.com_err = process_command.communicate(timeout=self.timeout)
        except subprocess.CalledProcessError as err:
            self.return_code, self.com_out, self.com_err = err.returncode, "", err
        except subprocess.TimeoutExpired:
            self.return_code, self.com_out, self.com_err = 124, "", "timeout"
        else:
            self.return_code = process_command.returncode
            self.com_out = self.com_out.decode(locale.getpreferredencoding(False), errors="ignore").strip()
            self.com_err = self.com_err.decode(locale.getpreferredencoding(False), errors="ignore").strip()
        finally:
            process_command.kill()
            try:
                os.killpg(process_command.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass

    def get_output(self):
        return self.com_out, self.com_err, self.return_code
