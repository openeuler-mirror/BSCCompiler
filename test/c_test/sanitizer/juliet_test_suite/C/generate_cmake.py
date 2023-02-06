#!/usr/bin/python3
import os
import sys
import re


testcases_dir = './testcases'
testcasesupport_dir = './testcasesupport'
cmake_build = 'cmake_build_O0'

util_src_list = [os.path.join(testcasesupport_dir, 'io.c'),
                 os.path.join(testcasesupport_dir, 'std_thread.c')]
util_src_list = ' '.join(util_src_list)

memory_cases = [
    "CWE121_Stack_Based_Buffer_Overflow",
    "CWE122_Heap_Based_Buffer_Overflow",
    "CWE124_Buffer_Underwrite",
    "CWE126_Buffer_Overread",
    "CWE127_Buffer_Underread"]


class Testcase:
    def __init__(self, cwe, sub_path, source, output, isC):
        self.cwe = cwe
        self.sub_path = sub_path
        self.source = source
        self.output = output
        self.isC = isC

    def __lt__(self, other):
        return self.output < other.output


def get_testcases(CWE):
    testcases = []
    sub_directory = os.listdir(os.path.join(testcases_dir, CWE))
    sub_directory.sort()

    Total_C_Num = 0
    Total_CPP_Num = 0
    Total_Out_Num = 0

    for sub_files in sub_directory:
        if sub_files in ['CMakeLists.txt', 'Makefile', 'cmake_install.cmake', 'CMakeCache.txt', 'CMakeFiles']:
            continue
        home = os.path.join(testcases_dir, CWE)
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
            elif filename.endswith('.c'):
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

    testcases.sort()
    return testcases


def get_testcase_list_for_cmake():
    cmake_lines = []
    cmake_lines.append(f'include_directories({testcases_dir} {testcasesupport_dir})')
    cmake_lines.append(f'add_library(libsupport OBJECT {util_src_list})')
    for CWE in memory_cases:
        testcases = get_testcases(CWE)
        for case in testcases:
            src_args = ' '.join(case.source)
            cmake_lines.append(f'add_executable({case.output} {src_args} $<TARGET_OBJECTS:libsupport>)')
    content = '\n'.join(cmake_lines)
    return content


def get_cmake_routines():
    lines = [
        'cmake_minimum_required(VERSION 3.23.0)',
        'project(juliet-testcases)',
        ''
    ]
    return '\n'.join(lines)


def create_clang_cmake():
    build_dir = os.path.join(cmake_build, 'clang')
    # NOTE: replace the clang path to your own clang
    # the clang must be compiled with compiler-rt enabled
    # we use the following command to configure llvm with cmake, gcc-7.5 and g++-7.5
    # cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_RUNTIMES=compiler-rt ../../llvm-12.0.0.src/llvm CC=g
    cc = '/root/git/ThirdParty/llvm-12.0.0-personal/build/bin/clang'
    # flags = f"-fsanitize=address -DINCLUDEMAIN -lstdc++ -lpthread -I{testcasesupport_dir}"
    flags = f"-O0 -fsanitize=address -DINCLUDEMAIN -lstdc++ -lpthread -Wno-unused-command-line-argument"
    cmake_lines = [
        '',
        f'set(CMAKE_C_COMPILER "{cc}")',
        f'set(CMAKE_C_FLAGS "{flags}")',
        f'set(CMAKE_CXX_COMPILER "{cc}")',
        f'set(CMAKE_CXX_FLAGS "{flags}")',
        ''
    ]
    content = get_cmake_routines()
    content += '\n'.join(cmake_lines) + '\n'
    content += get_testcase_list_for_cmake()
    os.makedirs(build_dir)
    with open(os.path.join(build_dir, 'CMakeLists.txt'), 'w') as of:
        of.write(content)


def create_arkcc_cmake():
    build_dir = os.path.join(cmake_build, 'maple')
    cc = os.path.realpath('./arkcc.py')
    flags = f" -DINCLUDEMAIN -lpthread -I{testcasesupport_dir}"
    cmake_lines = [
        f'set(CMAKE_C_COMPILER "{cc}")',
        f'set(CMAKE_C_FLAGS "{flags}")',
        f'set(CMAKE_CXX_COMPILER "{cc}")',
        f'set(CMAKE_CXX_FLAGS "{flags}")',
        ''
    ]
    content = get_cmake_routines()
    content += '\n'.join(cmake_lines) + '\n\n'
    content += get_testcase_list_for_cmake()
    os.makedirs(build_dir)
    with open(os.path.join(build_dir, 'CMakeLists.txt'), 'w') as of:
        of.write(content)


def create_arkcc_san_cmake():
    build_dir = os.path.join(cmake_build, 'maple_san')
    cc = os.path.realpath('./arkcc_asan.py')
    flags = f" -DINCLUDEMAIN -lpthread -I{testcasesupport_dir}"
    cmake_lines = [
        f'set(CMAKE_C_COMPILER "{cc}")',
        f'set(CMAKE_C_FLAGS "{flags}")',
        f'set(CMAKE_CXX_COMPILER "{cc}")',
        f'set(CMAKE_CXX_FLAGS "{flags}")',
        ''
    ]
    content = get_cmake_routines()
    content += '\n'.join(cmake_lines) + '\n\n'
    content += get_testcase_list_for_cmake()
    os.makedirs(build_dir)
    with open(os.path.join(build_dir, 'CMakeLists.txt'), 'w') as of:
        of.write(content)

def main():
    create_clang_cmake()
    # create_arkcc_cmake()
    # create_arkcc_san_cmake()


if __name__ == '__main__':
    main()
