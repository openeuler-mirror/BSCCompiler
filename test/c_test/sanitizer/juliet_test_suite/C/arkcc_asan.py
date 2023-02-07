#!/usr/bin/python3

import os
import sys

MAPLE_ROOT = os.environ['MAPLE_ROOT']
LINARO = f"{MAPLE_ROOT}/tools/gcc-linaro-7.5.0"
MAPLE_EXECUTE_BIN = f"{MAPLE_ROOT}/output/aarch64-clang-debug/bin"

LINARO_GCC = f"{LINARO}/bin/aarch64-linux-gnu-gcc"
SAVE_MAPLE_TEMPS = True
ASAN = True
ASAN_FLAG = "0x1"
OPT_LEVEL = "-O2"

juliet_include_dir=[
    f'-I{MAPLE_ROOT}/test/c_test/sanitizer/juliet_test_suite/C/testcasesupport',
    '-I/usr/aarch64-linux-gnu/include'
]

always_include_flags = ' '.join(juliet_include_dir)

# include_main = '-DINCLUDEMAIN'


def run_cmd(cmd):
    ret = os.system(cmd)
    # print(cmd)
    assert ret == 0, f"Fail {cmd}\nExit with {hex(ret) if ret > 0 else ('-' + hex(-ret))}."


def src2ast(src, flags):
    cmd = f"{MAPLE_ROOT}/tools/bin/clang --target=aarch64 -emit-ast {src} -o {src}.ast {flags} {always_include_flags} > stdout.txt 2> stderr.txt"
    run_cmd(cmd)


def ast2mpl(src):
    cmd = f"{MAPLE_EXECUTE_BIN}/hir2mpl {src}.ast -o {src}.mpl > stdout.txt 2> stderr.txt"
    run_cmd(cmd)


def mpl2ass(src):
    cmd = f"{MAPLE_EXECUTE_BIN}/maple"
    if SAVE_MAPLE_TEMPS:
        cmd += ' --save-temps'
    if ASAN:
        # cmd = f"{cmd} --run=me:mpl2mpl:mplcg --option=\"{OPT_LEVEL} --san={ASAN_FLAG}:{OPT_LEVEL} --no-inline -quiet:{OPT_LEVEL}\" {src}.mpl -o {src}.s > stdout.txt 2> stderr.txt"
        # NOTE: we set mplcg -O1 (both O2 and O0 can cause problem)
        cmd = f"{cmd} --run=me:mpl2mpl:mplcg --option=\"{OPT_LEVEL} --san={ASAN_FLAG}:{OPT_LEVEL} --no-inline -quiet:-O1\" {src}.mpl -o {src}.s > stdout.txt 2> stderr.txt"
    else:
        cmd = f"{cmd} --run=me:mpl2mpl:mplcg --option=\"{OPT_LEVEL}:{OPT_LEVEL} -quiet:{OPT_LEVEL}\" {src}.mpl -o {src}.s > stdout.txt 2> stderr.txt"
    run_cmd(cmd)


def ass2obj(src, dest):
    cmd = f"{LINARO_GCC} -c -o {dest} {src}.s"
    run_cmd(cmd)


def move_tmp_files(src, dest):
    # the dest is the output dest file, get by reading the -o option's argument
    dest_dir = os.path.dirname(dest)
    src_dir = os.path.dirname(src)
    if len(src_dir) == 0:
        src_dir = '.'
    if len(dest_dir) == 0:
        dest_dir = '.'
    if src_dir == dest_dir:
        # work in the same dir, no need to mov
        return
    tmp_ext_list = ['.ast', '.mpl', '.me.mpl', '.s']
    for tmp_ext in tmp_ext_list:
        tmp_file = f'{src}{tmp_ext}'
        if os.path.isfile(tmp_file):
            # run_cmd(f'mv {tmp_file} {dest_dir}/')
            # they use too much disks.
            run_cmd(f'rm {tmp_file}')


def add_asan_flags(flags):
    tokens = flags.split()
    necessary_flags = ['-lasan', '-lubsan', '-ldl', '-lpthread', '-lm', '-lrt']
    for flag in necessary_flags:
        if flag not in tokens:
            tokens.append(flag)
    return ' '.join(tokens)


def raw_run_gcc(dest, flags):
    if ASAN:
        flags = add_asan_flags(flags)
    cmd = f"{LINARO_GCC} {flags} -o {dest}"
    run_cmd(cmd)


def compile_only(src, dest, flags):
    src2ast(src, flags)
    ast2mpl(src)
    mpl2ass(src)
    ass2obj(src, dest)
    move_tmp_files(src, dest)


def trim_suffix(src_file):
    if src_file.endswith('.c'):
        return src_file[:-2]
    elif src_file.endswith('.cc'):
        return src_file[:-3]
    elif src_file.endswith('.cpp'):
        return src_file[:-4]
    assert False, f"{src_file} has unknown suffix!"


def compile_and_link(is_compile, src_list, dest, flags):
    # compile every src, then link
    obj_list = []
    for src in src_list:
        tmp_dest = f'{src}.o'
        compile_only(src, tmp_dest, flags)
        obj_list.append(tmp_dest)
        move_tmp_files(src, dest)
    # assemble together

    if not is_compile:
        if len(dest) == 0:
            dest = trim_suffix(src_list[0])
        flags = ' '.join(obj_list) + ' ' + flags
        raw_run_gcc(dest, flags)
    else:
        assert len(obj_list) == 1
        if len(dest) == 0:
            dest = trim_suffix(src_list[0]) + '.o'
        if dest != obj_list[0]:
            cmd = f"mv {obj_list[0]} {dest}"
            run_cmd(cmd)


def analyze_arguments():
    is_compile = False
    output_file = ''
    other_flags = []
    argv = sys.argv
    source_files = []
    valid = [True] * len(argv)
    idx = 1
    while idx < len(argv):
        if argv[idx] == '-c':
            is_compile = True
            valid[idx] = False
        elif argv[idx] == '-o':
            output_file = argv[idx + 1]
            valid[idx] = False
            valid[idx + 1] = False
            idx += 1
        elif argv[idx].endswith(('.c', '.cc', '.cpp', '.h')):
            source_files.append(argv[idx])
            valid[idx] = False
        idx += 1

    for idx in range(1, len(argv)):
        if valid[idx]:
            other_flags.append(argv[idx])
    other_flags = ' '.join(other_flags)
    return is_compile, source_files, output_file, other_flags


def main():
    is_compile, src_list, dest, flags = analyze_arguments()
    # print(sys.argv)
    # print('SOURCE:', src_list)
    # print('OUTPUT:', dest)
    # print('FLAGS: ', flags)
    if len(src_list) == 0:
        raw_run_gcc(dest, flags)
        return
    compile_and_link(is_compile, src_list, dest, flags)


if __name__ == '__main__':
    main()

