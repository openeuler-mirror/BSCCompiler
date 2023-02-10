#!/bin/bash

workdir=`realpath $1`
util_name="$2"
current=`pwd`
# arkcc="$current/arkcc.py"
arkcc="$current/arkcc_asan.py"

runcmd(){
    cmd="cd $workdir && $@ && cd $current"
    echo $@
    eval $cmd
}

if [[ -f $workdir/Makefile ]]; then
    runcmd "make clean"
    rm $workdir/Makefile
fi

tmp="FORCE_UNSAFE_CONFIGURE=1 ./configure --prefix=$current/../install --without-selinux --host=arm-linux-gnueabihf CC=$arkcc CFLAGS='-O0 -g' > /dev/null"
echo "configuring ..."
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
    echo "Compile $util_name successfully."
else
    echo "Fail to compile $util_name"
fi

