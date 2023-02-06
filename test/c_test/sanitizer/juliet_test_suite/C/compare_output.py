#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys


timeout_error = []
good_exit = []
finished_all = []
finished_all_with_ASAN = []


def readlines_skip_exception(path):
    lines = []
    with open(path, 'rb') as f:
        content = f.read()
        # there may be some bytes without proper encoding, skip them
        tmp = content.split(b'\n')
        for t in tmp:
            try:
                t = str(t, encoding='utf-8')
                lines.append(t + '\n')
            except Exception as e:
                pass
    return lines


def read_output(path):
    # lines = open(path).readlines()
    lines = readlines_skip_exception(path)
    assert lines[0].startswith('Timeout:'), path + '\n' + lines[0]
    is_timeout = 2 if lines[0].strip().split()[-1] == 'True' else 0

    assert lines[1].startswith('UnknownError:'), path + '\n' + lines[1]
    is_error = 2 if lines[1].strip().split()[-1] == 'True' else 0
    if is_timeout or is_error:
        # print(f'{is_timeout} {is_error}', path)
        timeout_error.append(path)
        return [is_timeout, is_error, 0, 0, 0, 0]

    content = ''.join(lines[2:])
    invoke_asan = content.find('AddressSanitizer') >= 0
    calling_good = content.find('Calling good()') >= 0
    finished_good = content.find('Finished good()') >= 0
    calling_bad = content.find('Calling bad()') >= 0
    finished_bad = content.find('Finished bad()') >= 0
    TP, FP, TN, FN = 0, 0, 0, 0
    if calling_good and finished_good:
        TN = 1
    if invoke_asan:
        if calling_bad and not finished_bad:
            TP = 1
        elif calling_good and not finished_good:
            FP = 1
            FN = 1
            good_exit.append(path)
            # print("Trigger ASAN calling good: ", path)
        else:
            # sometimes the ASAN knows an error, but the instrumentation is not triggered
            if calling_good and calling_bad and finished_bad:
                FN = 1
                # print("Trigger ASAN with all finished: ", path)
                finished_all_with_ASAN.append(path)
            else:
                assert False, path
    else:
        if calling_good and calling_bad and finished_bad:
            FN = 1
            # print("All finished normally: ", path)
            finished_all.append(path)
        else:
            assert False, path
    return [is_timeout, is_error, TP, FP, TN, FN]


def read_output_dir(dir_path):
    files = os.listdir(dir_path)
    files = sorted(files)
    files = map(lambda f: os.path.join(dir_path, f), files)
    count = [0, 0, 0, 0, 0, 0]
    total = 0
    for f in files:
        tmp = read_output(f)
        total += 2
        for idx in range(len(count)):
            count[idx] += tmp[idx]
    return count, total


def trim_prefix(path, prefix):
    if prefix is None or len(prefix) == 0:
        return path
    if path.startswith(prefix):
        return path[len(prefix):]
    else:
        assert False, f"{path} with prefix\n{prefix}"


def print_output_res(res, omit_prefix=None):
    for p in timeout_error:
        p = trim_prefix(p, omit_prefix)
        print('TO/ERR', p)
    for p in good_exit:
        p = trim_prefix(p, omit_prefix)
        print('Exit calling good', p)
    for p in finished_all:
        p = trim_prefix(p, omit_prefix)
        print('Finished all normally', p)
    for p in finished_all_with_ASAN:
        p = trim_prefix(p, omit_prefix)
        print('Finished all with ASAN', p)
    print(f'Total:   {res[1]}')
    print(f'Timeout: {res[0][0]}')
    print(f'ERR:     {res[0][1]}')
    print(f'TP:      {res[0][2]}')
    print(f'TN:      {res[0][4]}')
    print(f'FP:      {res[0][3]}')
    print(f'FN:      {res[0][5]}')


def main():
    output_dir = sys.argv[1]
    res = read_output_dir(output_dir)
    print_output_res(res, output_dir)


if __name__ == '__main__':
    main()

