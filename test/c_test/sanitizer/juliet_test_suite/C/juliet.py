#!/usr/bin/python3

import os
import re
import sys
import time
import subprocess
import colorama
from colorama import Fore, Style
import tqdm
import time


work_dir = os.path.dirname(os.path.realpath(__file__))
maple_root = os.environ['MAPLE_ROOT']

testcase_dir = os.path.realpath(os.path.join(work_dir, 'testcases'))
testcasesupport_dir = os.path.realpath(os.path.join(work_dir, 'testcasesupport'))

output_dir = os.path.realpath(os.path.join(work_dir, 'ark_test'))
clang_output_dir = os.path.realpath(os.path.join(output_dir, 'clang'))
maple_output_dir = os.path.realpath(os.path.join(output_dir, 'maple'))
maple_asan_output_dir = None

memory_cases = [
    "CWE121_Stack_Based_Buffer_Overflow",
    "CWE122_Heap_Based_Buffer_Overflow",
    "CWE124_Buffer_Underwrite",
    "CWE126_Buffer_Overread",
    "CWE127_Buffer_Underread"]

arkcc = os.path.realpath(os.path.join(work_dir, 'arkcc.py'))
arkcc_asan = os.path.realpath(os.path.join(work_dir, 'arkcc_asan.py'))
gcc_linaro_lib = os.path.realpath(os.path.join(maple_root, "tools/gcc-linaro-7.5.0/aarch64-linux-gnu/lib64"))
asan_dynamic_lib = os.path.join(gcc_linaro_lib, 'libasan.so')
clang = '/root/git/ThirdParty/llvm-12.0.0-personal/build/bin/clang'

testcases = []


class Testcase:
    def __init__(self, cwe, sub_path, source, output, isC):
        self.cwe = cwe
        self.sub_path = sub_path
        self.source = source
        self.output = output
        self.isC = isC

    def __lt__(self, other):
        return self.output < other.output


class cd:
    def __init__(self, newPath):
        self.newPath = os.path.expanduser(newPath)

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.makedirs(self.newPath, exist_ok=True)
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)


def cmd(commandline):
    print(commandline)
    status, output = subprocess.getstatusoutput(commandline)
    if status:
        print(Fore.RED + output, file=sys.stderr)
        exit(1)
    return status, output


# def timeout_run(cmd_str: str, timeout=1):
#     try:
#         # https://stackoverflow.com/a/4459103
#         cmd_str = 'script -qc "{}" &> temp'.format(cmd_str)
#         proc = subprocess.Popen(cmd_str, shell=True)
#         t_begin = time.time()
#
#         while True:
#             if proc.poll() is not None:
#                 break
#             t_passed = time.time() - t_begin
#             if timeout and t_passed > timeout:
#                 proc.terminate()
#                 msg = "Timeout: Command '" + cmd_str + "' timed out after " + str(timeout) + " seconds"
#                 raise Exception(msg)
#             time.sleep(0.1)
#         with open("temp", "r", encoding='unicode_escape') as f:
#             return True, f.read()
#     except Exception as e:
#         return False, msg


def timeout_run(cmd_str: str, timeout=1):
    try:
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
        return True, output
    except subprocess.TimeoutExpired as e:
        msg = "Timeout: Command '" + cmd_str + "' timed out after " + str(timeout) + " seconds"
        return False, msg
    except Exception as e:
        msg = "UnknowError:" + os.getcwd() + ": Command '" + cmd_str + "' raise " + str(e)
        return False, msg


def maple_compile(src_files: list, output: str, asan: bool, rbtree: bool):
    if asan:
        tmp = [arkcc_asan, '-DINCLUDEMAIN'] + src_files + ['-o', output, '-lpthread']
    else:
        tmp = [arkcc, '-DINCLUDEMAIN'] + src_files + ['-o', output, '-lpthread']
    status, output = cmd(' '.join(tmp))


def maple_compile_only(source: str, asan: bool):
    (filepath, tempfilename) = os.path.split(source)
    (filename, extension) = os.path.splitext(tempfilename)
    output = f'{filename}.o'
    if asan:
        tmp = [arkcc_asan, '-c', source] + ['-o', output]
    else:
        tmp = [arkcc, '-c', source] + ['-o', output]
    cmd(' '.join(tmp))
    return output


def compile(compiler: str):
    if compiler == "clang":
        for case in testcases:
            with cd(os.path.join(clang_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                if case.isC:
                    clang_cmd = "{} -fsanitize=address -DINCLUDEMAIN -lstdc++ -I {} {}/io.c {}/std_thread.c {} -o {}".format(clang, testcasesupport_dir, testcasesupport_dir, testcasesupport_dir, " ".join(case.source), case.output)
                    cmd(clang_cmd)
    if compiler == "maple":
        with cd(maple_output_dir):
            header_objs = []
            header_objs.append(os.path.realpath(maple_compile_only("{}/io.c".format(testcasesupport_dir), False)))
            header_objs.append(os.path.realpath(maple_compile_only("{}/std_thread.c".format(testcasesupport_dir), False)))
        for case in testcases:
            with cd(os.path.join(maple_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                if case.isC:
                    tmp_src = header_objs.copy()
                    tmp_src.extend(case.source)
                    maple_compile(tmp_src, case.output, False, False)
    if compiler.startswith("maple_asan"):
        with cd(maple_asan_output_dir):
            header_objs = []
            header_objs.append(os.path.realpath(maple_compile_only("{}/io.c".format(testcasesupport_dir), True)))
            header_objs.append(os.path.realpath(maple_compile_only("{}/std_thread.c".format(testcasesupport_dir), True)))
        for case in testcases:
            with cd(os.path.join(maple_asan_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                if case.isC:
                    tmp_src = header_objs.copy()
                    tmp_src.extend(case.source)
                    maple_compile(tmp_src, case.output, True, compiler.endswith('_rbtree'))


def fix(compiler: str):
    if compiler == "clang":
        pass
    if compiler == "maple":
        pass
    if compiler.startswith("maple_asan"):
        for case in testcases:
            with cd(os.path.join(maple_asan_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                if case.isC:
                    maple_compile_sub(case, True, compiler.endswith('_rbtree'))


def get_current_time():
    tmp = time.localtime()
    tmp = time.strftime("%D-%H-%M-%S", tmp)
    return tmp.replace('/', '-')


def run(compiler: str):
    current_time = get_current_time()
    if compiler == "clang":
        log_file_path = os.path.join(clang_output_dir, f'{current_time}.log')
        log_file = open(log_file_path, 'w')
        for case in tqdm.tqdm(testcases):
            with cd(os.path.join(clang_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                run_cmd = "./{}".format(case.output)
                isNotTimeout, output = timeout_run(run_cmd, timeout=5)
                # isNotTimeout, msg = True, "AddressSanitizer"
                print(case.output, file=log_file)

                if isNotTimeout and output.find("AddressSanitizer") != -1:
                    print(isNotTimeout, "AddressSanitizer", file=log_file)
                else:
                    print(isNotTimeout, "NoAddressSanitizer", file=log_file)
                for line in output.split('\n'):
                    print(line, file=log_file)
                    # if line.find('SUMMARY:') != -1:
                    #     break
                    # elif line.find('#') != -1 or line.find('HINT:') != -1:
                    #     continue
                    # elif line.find('located') != -1:
                    #     if line.find('region') != -1:
                    #         print(line[line.find('located'):line.find('region') + 6])
                    #     else:
                    #         print(line[line.find('located'):])
                    # elif line.find('ERROR') != -1 and line.find('address') != -1:
                    #     print(line[line.find('ERROR'): line.find('address') + 7])
                    # elif line.find('ERROR') != -1 and line.find('memory ranges') != -1:
                    #     print(line[line.find('ERROR'): line.find('memory ranges') + 13])
                    # elif line.find('thread') != -1 and line.find('at') != -1:
                    #     print(line[:line.find('at')])
                    # else:
                    #     print(line)
        log_file.close()
    if compiler == "maple":
        log_file_path = os.path.join(maple_output_dir, f'{current_time}.log')
        log_file = open(log_file_path, 'w')
        for case in tqdm.tqdm(testcases):
            with cd(os.path.join(maple_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                run_cmd = "/usr/bin/qemu-aarch64 -L /usr/aarch64-linux-gnu {}".format(case.output)
                isNotTimeout, output = timeout_run(run_cmd, timeout=5)
                print(case.output, file=log_file)
                print(output, file=log_file)
        log_file.close()
    if compiler.startswith("maple_asan"):
        log_file_path = os.path.join(maple_asan_output_dir, f'{current_time}.log')
        log_file = open(log_file_path, 'w')
        for case in tqdm.tqdm(testcases):
            with cd(os.path.join(maple_asan_output_dir, re.findall("CWE[0-9]*", case.cwe)[0])):
                if compiler.endswith('_rbtree'):
                    run_cmd = "ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-10 ASAN_OPTIONS=detect_leaks=0 qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_LIBRARY_PATH={}/src/runtime/libasan_rbt/build/compiler_rt/lib/linux:{} {}".format(maple_root, gcc_linaro_lib, case.output)
                else:
                    run_cmd = "ASAN_OPTIONS=detect_leaks=0 qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_PRELOAD={} -E LD_LIBRARY_PATH={} {}".format(asan_dynamic_lib, gcc_linaro_lib, case.output)
                isNotTimeout, output = timeout_run(run_cmd, timeout=10)
                print(case.output, file=log_file)

                if isNotTimeout and output.find("AddressSanitizer") != -1:
                    print(isNotTimeout, "AddressSanitizer", file=log_file)
                    if output.find('Calling bad()...') != -1 and output.find('Finished bad()') == -1:
                        print(case.output, 'TRUE POSITIVE', file=log_file)
                    else:
                        print(case.output, 'FALSE POSITIVE', file=log_file)
                else:
                    print(isNotTimeout, "NoAddressSanitizer", file=log_file)
                    if output.find('Calling') != -1:
                        print(case.output, 'FALSE NEGATIVE', file=log_file)
                    else:
                        print(case.output, 'ERROR', file=log_file)
                for line in output.split('\n'):
                    print(line, file=log_file)
                    # if line.find('SUMMARY:') != -1:
                    #     break
                    # elif line.find('#') != -1 or line.find('HINT:') != -1:
                    #     continue
                    # elif line.find('located') != -1:
                    #     if line.find('region') != -1:
                    #         print(line[line.find('located'):line.find('region') + 6])
                    #     else:
                    #         print(line[line.find('located'):])
                    # elif line.find('ERROR') != -1 and line.find('address') != -1:
                    #     print(line[line.find('ERROR'): line.find('address') + 7])
                    # elif line.find('ERROR') != -1 and line.find('memory ranges') != -1:
                    #     print(line[line.find('ERROR'): line.find('memory ranges') + 13])
                    # elif line.find('thread') != -1 and line.find('at') != -1:
                    #     print(line[:line.find('at')])
                    # else:
                    #     print(line)
        log_file.close()


def info(compiler: str):
    for case in testcases:
        print(case.output)


def main(compiler, mode, testcase, prefix):
    for CWE in memory_cases:
        if testcase != "all":
            if not CWE.startswith(testcase):
                continue
        sub_directory = os.listdir(os.path.join(testcase_dir, CWE))
        sub_directory.sort()

        Total_C_Num = 0
        Total_CPP_Num = 0
        Total_Out_Num = 0

        for sub_files in sub_directory:
            if sub_files in ['CMakeLists.txt', 'Makefile', 'cmake_install.cmake', 'CMakeCache.txt', 'CMakeFiles']:
                continue
            home = os.path.join(testcase_dir, CWE)
            files = os.listdir(os.path.join(home, sub_files))
            files.sort()
            home = os.path.join(home, sub_files)

            executables = set()
            outfiles = set()

            C_Num = 0
            CPP_Num = 0
            Out_Num = 0

            for filename in files:
                if not filename.startswith(CWE):
                    continue
                elif filename.find('wchar_t') != -1:
                    continue
                elif filename.find('fscanf') != -1:
                    continue
                elif filename.find('socket') != -1:
                    continue
                elif filename.find('fgets') != -1:
                    continue
                elif filename.find('rand') != -1:
                    continue
                elif filename.find('_12') != -1:
                    continue
                # elif filename.find('_34') != -1:
                #     continue
                # elif filename.find('_67') != -1:
                #     continue
                elif filename.startswith('CWE126_Buffer_Overread__CWE170_char'):
                    continue
                elif filename.endswith('.c') and filename.startswith(prefix):
                    executable_path = re.findall(CWE + ".*_[0-9]+", filename)[0]
                    executables.add(executable_path)
                    C_Num += 1
                elif filename.endswith('cpp'):
                    # executable_path = re.findall(CWE + ".*_[0-9]+", filename)[0]
                    # executables.add(executable_path)
                    CPP_Num += 1
                # elif filename.endswith('.out'):
                #     Out_Num += 1
                #     outfiles.add(os.path.splitext(filename)[0])
                else:
                    continue

            Total_C_Num += C_Num
            Total_CPP_Num += CPP_Num
            Total_Out_Num += Out_Num

            for executable in executables:
                source = [os.path.join(home, filename) for filename in files if filename.startswith(executable) and filename.endswith('.c')]
                if len(source) != 0:
                    testcases.append(Testcase(CWE, home, source, executable + ".out", True))
                else:
                    source = [os.path.join(home, filename) for filename in files if filename.startswith(executable) and filename.endswith('.cpp')]
                    testcases.append(Testcase(CWE, home, source, executable + ".out", False))

        # print('{} Total C_count {}, Total CPP_count {}, Total Out_count {}\n'.format(home, Total_C_Num, Total_CPP_Num, Total_Out_Num))
    testcases.sort()
    if mode == "compile":
        compile(compiler)
    if mode == "run":
        run(compiler)
    if mode == "info":
        info(compiler)
    if mode == "fix":
        fix(compiler)


if __name__ == '__main__':
    if len(sys.argv) == 5:
        mode = sys.argv[1]
        compiler = sys.argv[2]
        testcase = sys.argv[3]
        prefix = sys.argv[4]
        if compiler.startswith('maple_asan'):
            maple_asan_output_dir = os.path.realpath(os.path.join(output_dir, compiler))
        main(compiler, mode, testcase, prefix)
