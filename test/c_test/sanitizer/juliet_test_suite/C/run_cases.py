#!/usr/bin/python3.8
# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import multiprocessing as mp
import tqdm


work_dir = os.path.dirname(os.path.realpath(__file__))
mode = sys.argv[1]
bin_dir = sys.argv[2]
output_dir = sys.argv[3]

g_timeout = 5
maple_root = os.environ['MAPLE_ROOT']
gcc_linaro_lib = os.path.realpath(os.path.join(maple_root, "tools/gcc-linaro-7.5.0/aarch64-linux-gnu/lib64"))
asan_dynamic_lib = os.path.join(gcc_linaro_lib, 'libasan.so')


def timeout_run(cmd_str: str, timeout=g_timeout):
    try:
        # it is werid that the stdout usually disappear
        # I have to use script to capture all output
        cmd_str = f'script -qc \"{cmd_str}\"'
        output = subprocess.run(cmd_str,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                check=False,
                                encoding='utf-8',
                                timeout=timeout,
                                shell=True).stdout
        # status, output = subprocess.getstatusoutput(cmd_str)
        if output is None:
            output = ''
        return False, False, output
    except subprocess.TimeoutExpired as e:
        msg = "Timeout: Command '" + cmd_str + "' timed out after " + str(timeout) + " seconds"
        return True, False, msg
    except Exception as e:
        msg = "UnknownError:" + os.getcwd() + ": Command '" + cmd_str + "' raise " + str(e)
        return False, True, msg


def timeout_run2(cmd_str: str, timeout=g_timeout):
    try:
        # it is werid that the stdout usually disappear
        # I have to use script to capture all output
        cmd_str = f'script -qc \"{cmd_str}\"'
        output = subprocess.run(cmd_str,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                check=False,
                                # we return output bytes directly
                                # encoding='utf-8',
                                timeout=timeout,
                                shell=True).stdout
        # status, output = subprocess.getstatusoutput(cmd_str)
        if output is None:
            output = b''
        return False, False, output
    except subprocess.TimeoutExpired as e:
        msg = b"Timeout: Command '" + cmd_str + "' timed out after " + str(timeout) + " seconds"
        return True, False, msg
    except Exception as e:
        msg = b"UnknownError:" + os.getcwd() + ": Command '" + cmd_str + "' raise " + str(e)
        return False, True, msg


def dump_msg(is_timeout, is_error, msg, out_path):
    with open(out_path, 'w') as of:
        content = f"Timeout: {is_timeout}\nUnknownError: {is_error}\n"
        content += msg
        print(content, file=of)


def dump_msg_bytes(is_timeout, is_error, msg, out_path):
    with open(out_path, 'wb') as of:
        content = f"Timeout: {is_timeout}\nUnknownError: {is_error}\n"
        content = content.encode(encoding='utf-8')
        content += msg
        of.write(content)


RUN_FUNC = timeout_run2
DUMP_FUNC = dump_msg_bytes

def run_clang(bin_path, out_path):
    cmd = bin_path
    if not cmd.startswith('/'):
        cmd = 'ASAN_OPTIONS=detect_leaks=0 ./' + cmd
    is_timeout, is_error, msg = RUN_FUNC(cmd)
    DUMP_FUNC(is_timeout, is_error, msg, out_path)


def run_maple(bin_path, out_path):
    # if not bin_path.startswith('/'):
    #     bin_path = './' + bin_path
    cmd = "ASAN_OPTIONS=detect_leaks=0 qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_PRELOAD={} -E LD_LIBRARY_PATH={} {}".format(asan_dynamic_lib, gcc_linaro_lib, bin_path)
    is_timeout, is_error, msg = RUN_FUNC(cmd)
    DUMP_FUNC(is_timeout, is_error, msg, out_path)


def main():
    run_f = None
    if mode == 'clang':
        run_f = run_clang
    elif mode == 'maple':
        run_f = run_maple
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    else:
        if mode == 'maple-c':
            run_f = run_maple
        else:
            assert False, "Use an empty directory, or use `maple-c`"
    bin_name_list = os.listdir(bin_dir)
    bin_name_list = list(filter(lambda n: n.endswith('.out'), bin_name_list))
    out_path_list = list(map(lambda n: os.path.join(output_dir, n), bin_name_list))
    bin_path_list = list(map(lambda n: os.path.join(bin_dir, n), bin_name_list))

    args_list = list(zip(bin_path_list, out_path_list))
    if mode == 'maple-c':
        # we merely run the cases without result
        args_list = list(filter(lambda i: not os.path.exists(i[1]), args_list))
    with mp.Pool(processes=16) as pool:
        pool.starmap(run_f, args_list)
    # for i in tqdm.tqdm(args_list):
    #     run_f(i[0], i[1])


if __name__ == '__main__':
    main()

