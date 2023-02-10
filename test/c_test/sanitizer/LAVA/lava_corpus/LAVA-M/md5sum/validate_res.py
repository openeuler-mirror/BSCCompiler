#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys

groundtruth_dir = 'clang_result'
output_dir = sys.argv[1]


def triggered_ASAN(lines):
    for l in lines:
        if 'AddressSanitizer' in l:
            return True
    return False


def triggered_bugs(lines):
    lines = filter(lambda l: l.startswith('Successfully triggered bug'), lines)
    bug_ids = dict()
    for l in lines:
        bug_id = int(l.split()[3].strip(','))
        if bug_id not in bug_ids:
            bug_ids[bug_id] = 1
        else:
            bug_ids[bug_id] += 1
    return bug_ids


def get_info(res_dir):
    ret = []
    for id in sorted(os.listdir(res_dir)):
        fpath = os.path.join(res_dir, id)
        lines = open(fpath).readlines()
        asan = triggered_ASAN(lines)
        bugs = triggered_bugs(lines)
        ret.append((asan, bugs))
    return zip(*ret)


def main():
    input_ids = sorted(os.listdir(groundtruth_dir))
    asans1, bugs1 = get_info(output_dir)
    asans2, bugs2 = get_info(groundtruth_dir)
    # if asan triggered
    FP, FN, TP, TN = 0, 0, 0, 0
    for idx in range(len(input_ids)):
        if asans1[idx] != asans2[idx]:
            if asans1[idx]:
                FP += 1
            else:
                FN += 1
        else:
            if asans1[idx]:
                TP += 1
            else:
                TN += 1
    print(f"ASAN Trigger")
    print(f"TP: {TP}")
    print(f"TN: {TN}")
    print(f"FP: {FP}")
    print(f"FN: {FN}")

    # if the bug reached
    FP, FN, TP, TN = 0, 0, 0, 0
    for idx in range(len(input_ids)):
        for bug_id in bugs1[idx]:
            n = bugs1[idx][bug_id] - bugs2[idx].get(bug_id, 0)
            if n == 0:
                TP += bugs1[idx][bug_id]
            elif n > 0:
                TP += bugs2[idx].get(bug_id, 0)
                FP += n
            else:
                TP += bugs1[idx][bug_id]
                FN -= n
    print("Bug Trigger")
    print(f"TP: {TP}")
    print(f"TN: {TN}")
    print(f"FP: {FP}")
    print(f"FN: {FN}")


if __name__ == '__main__':
    main()

