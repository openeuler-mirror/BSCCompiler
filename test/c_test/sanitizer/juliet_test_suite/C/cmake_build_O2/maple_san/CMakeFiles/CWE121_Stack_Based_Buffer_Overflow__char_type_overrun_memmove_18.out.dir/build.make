# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san

# Include any dependencies generated for this target.
include CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/flags.make

CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o: CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/flags.make
CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o: /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c
CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o: CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o"
	/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/arkcc_asan.py $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o -MF CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o.d -o CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o -c /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c

CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.i"
	/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/arkcc_asan.py $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c > CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.i

CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.s"
	/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/arkcc_asan.py $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c -o CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.s

# Object files for target CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out
CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18_out_OBJECTS = \
"CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o"

# External object files for target CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out
CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18_out_EXTERNAL_OBJECTS = \
"/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san/CMakeFiles/libsupport.dir/testcasesupport/io.c.o" \
"/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san/CMakeFiles/libsupport.dir/testcasesupport/std_thread.c.o"

CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out: CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/testcases/CWE121_Stack_Based_Buffer_Overflow/s01/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.c.o
CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out: CMakeFiles/libsupport.dir/testcasesupport/io.c.o
CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out: CMakeFiles/libsupport.dir/testcasesupport/std_thread.c.o
CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out: CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/build.make
CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out: CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/build: CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out
.PHONY : CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/build

CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/cmake_clean.cmake
.PHONY : CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/clean

CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/depend:
	cd /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san /root/git/OpenArkCompiler/test/c_test/sanitizer/juliet_test_suite/C/cmake_build_O2/maple_san/CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/CWE121_Stack_Based_Buffer_Overflow__char_type_overrun_memmove_18.out.dir/depend

