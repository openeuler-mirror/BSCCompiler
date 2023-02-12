#!/bin/bash

workdir=`realpath $1`
util_name="$2"
current=`pwd`
# arkcc="$current/arkcc.py"
arkcc="$current/arkcc_asan.py"

RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m'

runcmd(){
    cmd="cd $workdir && $@ && cd $current"
    echo -e "${GREEN}$@${NOCOLOR}"
    eval $cmd
}

if [[ -f $workdir/Makefile ]]; then
    runcmd "make clean > /dev/null"
    rm $workdir/Makefile
fi

# make sure the binary is removed
if [[ -f $workdir/src/$util_name ]]; then
    echo "${RED}Fail to clean the build directory, exit"
    exit 1
fi

CFLAGS="'-I/usr/aarch64-linux-gnu/include -O0 -g -DHAVE_MBSTATE=1 -DGNULIB_defined_wctrans_t -DGNULIB_defined_wctype_t'"

tmp="FORCE_UNSAFE_CONFIGURE=1 ./configure --prefix=$current/../install --without-selinux --host=arm-linux-gnueabihf CC=$arkcc CFLAGS=$CFLAGS > /dev/null"
echo -e "${GREEN}configuring ...${NOCOLOR}"
runcmd $tmp

# run make all first. this process will create many configured header files for the following compilation
runcmd "timeout 1 make &> /dev/null"

# make the target util
runcmd "make src/$util_name"

# the compilation will crash since libcoreutils.a and libver.a are not processed by ranlib
runcmd "ranlib lib/libcoreutils.a"
runcmd "ranlib src/libver.a"
# compile again
runcmd "make src/$util_name"

if [[ -f $workdir/src/$util_name ]]; then
    echo -e "${GREEN}Compile $util_name successfully."
    eval "cp $workdir/src/$util_name ./maple-install/$util_name-asan-O2"
else
    echo -e "${RED}Fail to compile $util_name"
fi

