#!/bin/bash

FILENAME=$1
arkcc=$MAPLE_ROOT/scripts/arkcc_asan.py

run_cmd(){
    cmd=$1
    echo $cmd
    eval $cmd
    if [ $? -ne 0 ]; then
        echo "failed"
        exit
    fi
}

# compile to object and insert asan logics
cmd="$arkcc $FILENAME -o $FILENAME.out"
run_cmd "$cmd"

# cmd="$GCC_LINARO_PATH/bin/aarch64-linux-gnu-gcc $FILENAME.s -o $FILENAME.out -lasan -ldl -lpthread -lm -lrt"
# run_cmd "$cmd"

# use qemu-aarch64 to test, note that the runtime library of asan could be replaced with clang's implementation with RBTree
cmd="ASAN_OPTIONS=detect_leaks=0 qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_LIBRARY_PATH=$GCC_LINARO_PATH/aarch64-linux-gnu/lib64 $FILENAME.out"
echo $cmd
eval $cmd

# clean tmp files
rm $FILENAME.ast $FILENAME.s $FILENAME.mpl $FILENAME.me.mpl $FILENAME.o comb.me.mpl comb.san.mpl
rm $FILENAME.out

