#!/bin/bash
#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

THIRD_PARTY_PATH=$MAPLE_ROOT/third_party
TOOLS_PATH=$MAPLE_ROOT/build/third_party
AOSP_PATH=$THIRD_PARTY_PATH/aosp_10.0.0_r35
TEMP_PATH=$THIRD_PARTY_PATH/temp
AOSP_GN_PATH=$TOOLS_PATH/aosp_gn

function install_patch {
    if [ -d $TEMP_PATH ];then
        echo "Already Patched."
        exit 0
    fi

    echo "Preparing the build environment..."

    #backup source code
    cd $THIRD_PARTY_PATH
    mkdir -p $TEMP_PATH
    tar -zcf aosp_10.0.0_r35.tar.gz aosp_10.0.0_r35/
    mv aosp_10.0.0_r35.tar.gz $TEMP_PATH/

    #patch
    cd $AOSP_PATH
    patch -p0 < $TOOLS_PATH/system_001.patch
    patch -p0 < $TOOLS_PATH/art_001.patch
    mkdir -p include/
    cp -r ${MAPLE_ROOT}/src/mplfe/dex_input/include/string_view_format.h include/

    #add third_party gn
    cp $AOSP_GN_PATH/art/libdexfile/BUILD.gn $AOSP_PATH/art/libdexfile/
    cp $AOSP_GN_PATH/system/core/libziparchive/BUILD.gn $AOSP_PATH/system/core/libziparchive/
    cp $AOSP_GN_PATH/system/core/base/BUILD.gn $AOSP_PATH/system/core/base/

}


function uninstall_patch {
    if [ ! -d $TEMP_PATH ];then
        exit 0
    fi

    cd $THIRD_PARTY_PATH
    rm -rf $AOSP_PATH
    mv $TEMP_PATH/aosp_10.0.0_r35.tar.gz .
    tar -zxvf aosp_10.0.0_r35.tar.gz > 0
    rm -rf $TEMP_PATH
    rm -rf aosp_10.0.0_r35.tar.gz
}

function main {
    if [ "x$1" == "xpatch" ]; then
        install_patch
    fi

    if [ "x$1" == "xunpatch" ]; then
        uninstall_patch
    fi
    cd $MAPLE_ROOT
}


main $@
