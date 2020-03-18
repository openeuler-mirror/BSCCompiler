#!/usr/bin/python3
# -*- coding:utf-8 -*-
#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under the Mulan PSL v1.
# You can use this software according to the terms and conditions of the Mulan PSL v1.
# You may obtain a copy of Mulan PSL v1 at:
#
#     http://license.coscl.org.cn/MulanPSL
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v1 for more details.
#
import os
import signal
import subprocess
import sys
import time
import timeit
from textwrap import indent, shorten

from maple_test.configs import construct_logger
from maple_test.utils import PASS, FAIL, UNRESOLVED, ENCODING
from maple_test.utils import add_run_path


class TestError(Exception):
    pass


def run_command(cmd, work_dir, timeout, logger, env=None):
    """Run commands using subprocess"""
    new_env = add_run_path(str(work_dir))
    new_env.update(env)
    process_command = subprocess.Popen(
        cmd,
        shell=True,
        cwd=str(work_dir),
        env=new_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        close_fds=True,
        start_new_session=True,
    )
    logger.debug("execute cmd ===>>>: %s", cmd)
    return_code = com_out = com_err = None
    try:
        com_out, com_err = process_command.communicate(timeout=timeout)
    except subprocess.CalledProcessError as err:
        return_code, com_out, com_err = err.returncode, "", err
        logger.exception(err)
        return return_code, com_out, com_err
    except subprocess.TimeoutExpired:
        return_code, com_out, com_err = 3, "TimeOut", "TimeOut"
        return return_code, com_out, com_err
    else:
        return_code = process_command.returncode
        com_out = com_out.decode(ENCODING, errors="ignore")
        com_err = com_err.decode(ENCODING, errors="ignore")
        return return_code, com_out, com_err
    finally:
        process_command.kill()
        try:
            os.killpg(process_command.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        logger.debug("return code: %d", return_code)
        logger.debug("stdout : \n%s", indent(com_out, "+\t", lambda line: True))
        logger.debug("stderr : \n%s", indent(com_err, "@\t", lambda line: True))


def run_commands(position, commands, work_dir, timeout, log_config, env=None):
    name = "{}_{}".format(log_config[1], int(time.time()))
    logger = construct_logger(log_config[0], name)
    remain_time = timeout
    result = (PASS, None)
    logger.debug("Work directory: {}".format(work_dir))
    if not commands:
        return position, (UNRESOLVED, None)
    for command in commands:
        start = timeit.default_timer()
        try:
            return_code, com_out, com_err = run_command(
                command, work_dir, remain_time, logger, env
            )
            if return_code != 0:
                result = (FAIL, (return_code, command, shorten(com_err, width=84)))
                err = "Run task exit unexpected : {}, Log file at: {}.log".format(
                    result[-1], log_config[0].get("dir") / name
                )
                raise TestError(err)
        except TestError as err:
            logger.error(err)
            break
        else:
            run_time = timeit.default_timer() - start
            remain_time = remain_time - run_time
            logger.debug(
                "Run time: {:.2}, remain time: {:.2}".format(run_time, remain_time)
            )
    if result[0] == PASS:
        logger.debug("Task executed successfully")
    handlers = logger.handlers[:]
    for handler in handlers:
        handler.close()
        logger.removeHandler(handler)
    return position, result


def progress(results, progress_type):
    """Output test progress"""

    if progress_type == "silent":
        return 0
    if progress_type == "normal":
        time_gape = 1
        print_progress = sys.stdout.write
    else:
        time_gape = 10
        print_progress = print
    finished = 0
    total = len(results)
    while total != finished:
        time.sleep(time_gape)
        finished = sum([result.ready() for result in results])
        rate = finished / total
        print_progress(
            "\rRunning test cases: {:.2%} ({}/{}) ".format(rate, finished, total)
        )
        sys.stdout.flush()
