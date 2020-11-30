#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

import argparse
import logging
import os
import shlex
import subprocess
import sys
import time
import uuid

# from assistant.SSH import SSH
from collections import OrderedDict
from textwrap import indent

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from maple_test.utils import ENCODING, complete_path, add_run_path

EXIT_CODE = 0


class MapleRunError(Exception):
    pass


def run(cmd, work_dir, timeout):
    process_command = subprocess.Popen(
        cmd,
        shell=True,
        cwd=str(work_dir),
        env=add_run_path(str(work_dir)),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        close_fds=True,
    )
    try:
        com_out, com_err = process_command.communicate(timeout=timeout)
    except subprocess.CalledProcessError as err:
        raise err
    else:
        return_code = process_command.returncode
        com_out = com_out.decode(ENCODING, errors="ignore")
        com_err = com_err.decode(ENCODING, errors="ignore")
        return return_code, com_out, com_err
    finally:
        process_command.terminate()

def pre_path(execute_cmd, execute_option):
    SSH_USER = execute_option["ssh_user"]
    SSH_IP = execute_option["ssh_ip"]
    SN = execute_option["sn"]
    remote_path = execute_option["remote_path"]
    android_path = execute_option["android_path"]
    args = "mkdir -p {remote_path}; adb -s {SN} shell mkdir -p {android_path}".format(SN=SN,
        remote_path=remote_path, android_path=android_path)
    execute_cmd["run_case"] = (
        '/usr/bin/ssh '
        '-o ControlMaster=auto -o ControlPath=~/.ssh/ssh-control-sock-%r@%h:%p -o ControlPersist=1h -o '
        'User={SSH_USER} -o StrictHostKeyChecking=no '
        '{SSH_IP} "{args}"'
        .format(
            SSH_USER=SSH_USER, SSH_IP=SSH_IP, args=args
        )
    )
    logging.debug("pre_path  "+execute_cmd["run_case"])

def scp_so_cmd(execute_cmd, execute_option):
    SSH_USER = execute_option["ssh_user"]
    SSH_IP = execute_option["ssh_ip"]
    remote_path = execute_option["remote_path"]
    android_path = execute_option["android_path"]
    execute_option["source_files"] = ":".join(
        [str(file) for file in execute_option["source_files"]]
    )
    execute_cmd["run_case"] = (
        "/usr/bin/scp "
        "-o ControlMaster=auto -o ControlPath=~/.ssh/ssh-control-sock-%r@%h:%p -o ControlPersist=1h -o "
        "User={SSH_USER} -o StrictHostKeyChecking=no {source_files} "
        "{SSH_IP}:{remote_path} >/dev/null".format(
            SSH_USER=SSH_USER, SSH_IP=SSH_IP, **execute_option
        )
    )
    logging.debug("scp_so_cmd  "+execute_cmd["run_case"])

def push_so_cmd(execute_cmd, execute_option):
    SSH_USER = execute_option["ssh_user"]
    SSH_IP = execute_option["ssh_ip"]
    SN = execute_option["sn"]
    remote_path = execute_option["remote_path"]
    android_path = execute_option["android_path"]
    execute_file = execute_option["execute_file"]
    args = "adb -s {SN} push {remote_path}{execute_file} {android_path};".format(SN=SN,
        execute_file=execute_file, remote_path=remote_path, android_path=android_path)
    execute_cmd["run_case"] = (
        '/usr/bin/ssh '
        '-o ControlMaster=auto -o ControlPath=~/.ssh/ssh-control-sock-%r@%h:%p -o ControlPersist=1h -o '
        'User={SSH_USER} -o StrictHostKeyChecking=no '
        '{SSH_IP} "{args}"'
        .format(
            SSH_USER=SSH_USER, SSH_IP=SSH_IP, args=args
        )
    )
    logging.debug("push_so_cmd  "+execute_cmd["run_case"])


def run_so_cmd(execute_cmd, execute_option):
    SSH_USER = execute_option["ssh_user"]
    SSH_IP = execute_option["ssh_ip"]
    SN = execute_option["sn"]
    remote_path = execute_option["remote_path"]
    android_path = execute_option["android_path"]
    execute_file = execute_option["execute_file"]
    execute_class = execute_option["execute_class"]
    args = "adb -s {SN} shell '  timeout 300 /system/bin/mplsh_arm64 -XX:HeapMaxFree=8m \
        -XX:HeapTargetUtilization=0.75 -Xbootclasspath:libcore-all.so \
        -cp {android_path}{execute_file} {execute_class}'".format(SN=SN, execute_file=execute_file,
        execute_class=execute_class, android_path=android_path)
    execute_cmd["run_case"] = (
        '/usr/bin/ssh '
        '-o ControlMaster=auto -o ControlPath=~/.ssh/ssh-control-sock-%r@%h:%p -o ControlPersist=1h -o '
        'User={SSH_USER} -o StrictHostKeyChecking=no '
        '{SSH_IP} "{args}"'
        .format(
            SSH_USER=SSH_USER, SSH_IP=SSH_IP, remote_path=remote_path, args=args
        )
    )
    logging.debug("run_so_cmd   "+execute_cmd["run_case"])


def parse_cli():
    parser = argparse.ArgumentParser(prog="android_run")
    parser.add_argument(
        "--run_type",
        default="aarch64",
        choices=["aarch64"],
        help="run type",
    )

    parser.add_argument(
        "execute_file", metavar="<file1>[:file2:file3...]", help="execute file, ",
    )
    parser.add_argument("execute_class", help="execute class")

    parser.add_argument("--execute_args", dest="execute_args", default="", help="execute args")
    parser.add_argument(
        "--timeout", help="run test case timeout", type=float, default=None
    )
    parser.add_argument(
        "--mrt_type",
        dest="mrt_type",
        default="O2",
        choices=["O0", "O2"],
        help="Add mrt type to the extra option",
    )

    parser.add_argument(
      "--ssh_user",
      dest="ssh_user",
      help="your ssh user"
    )

    parser.add_argument(
      "--ssh_ip",
      dest="ssh_ip",
      help="your ssh ip"
    )

    parser.add_argument(
      "--sn",
      dest="sn",
      help="your phone sn"
    )

    connection_options = parser.add_argument_group("Script options")
    connection_options.add_argument(
        "--verbose", action="store_true", dest="verbose", help="enable verbose output",
    )

    opts = parser.parse_args()
    return opts


def main():
    opts = parse_cli()
    run_type = opts.run_type

    parser = shlex.shlex(opts.execute_file)
    parser.whitespace = ":"
    parser.whitespace_split = True
    source_files = [complete_path(file) for file in parser]

    execute_file = opts.execute_file
    execute_class = opts.execute_class
    execute_args = opts.execute_args
    mrt_type = opts.mrt_type
    timeout = opts.timeout
    ssh_user = opts.ssh_user
    ssh_ip = opts.ssh_ip
    sn = opts.sn

    tmp_id = str(uuid.uuid1())
    remote_path = "~/ndk_test/tmp/" + execute_class + "_" + tmp_id + "/"
    android_path = "/data/local/ndk_test/tmp/" + execute_class + "_" + tmp_id + "/"

    execute_option = {
        "source_files": source_files,
        "execute_file" : execute_file,
        "execute_class" : execute_class,
        "execute_args": execute_args,
        "mrt_type": mrt_type,
        "timeout": timeout,
        "ssh_user": ssh_user,
        "ssh_ip": ssh_ip,
        "sn": sn,
        "remote_path": remote_path,
        "android_path": android_path,
    }

    logging.basicConfig(
        format="\t%(asctime)s %(message)s",
        datefmt="%H:%M:%S",
        level=logging.DEBUG if opts.verbose else logging.INFO,
        stream=sys.stderr,
    )

    execute_cmd = OrderedDict()
    execute_cmd["run_case"] = None

    # check_env_cmd(execute_cmd, execute_option)
    # isInit = run_case(execute_cmd, ".", timeout)
    # if isInit != 'yes':
    #     print(1234)
    #     init_env_cmd(execute_cmd, execute_option)
    #     run_case(execute_cmd, ".", timeout)

    pre_path(execute_cmd, execute_option)
    run_case(execute_cmd, ".", timeout)
    scp_so_cmd(execute_cmd, execute_option)
    run_case(execute_cmd, ".", timeout)
    push_so_cmd(execute_cmd, execute_option)
    run_case(execute_cmd, ".", timeout)
    run_so_cmd(execute_cmd, execute_option)
    run_case(execute_cmd, ".", timeout)


def run_case(execute_cmd, work_dir, timeout):
    for stage, cmd in execute_cmd.items():
        ret = run_cmd(stage, cmd, work_dir, timeout)
    return ret


def run_cmd(stage, cmd, work_dir, timeout):
    return_code, com_out, com_err = run(cmd, work_dir, timeout)
    logging.debug("execute command: %s", cmd)
    logging.debug("execute return code: %d", return_code)
    logging.debug("execute out: \n%s", indent(com_out, "\t", lambda line: True))
    logging.debug("execute error: \n%s", indent(com_err, "\t", lambda line: True))
    global EXIT_CODE
    EXIT_CODE = return_code
    print(com_out, end="")
    print(com_err, file=sys.stderr, end="")
    if return_code != 0:
        logging.error("execute command: %s", cmd)
        logging.error("execute return code: %d", return_code)
        logging.error("execute out: \n%s", indent(com_out, "\t", lambda line: True))
        logging.error(
            "execute error: \n%s", indent(com_err, "\t", lambda line: True)
        )
        reason = "Maple run stage: {} failed at command: {}, reason: {}".format(
            "run_case".upper(), cmd, com_err
        )
        raise MapleRunError(reason)
    return com_out

if __name__ == "__main__":
    try:
        main()
    except MapleRunError as e:
        sys.exit(EXIT_CODE)

