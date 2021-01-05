#!/bin/bash
#
# Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
#
# Licensed under the Mulan Permissive Software License v2.
# You can use this software according to the terms and conditions of the MulanPSL - 2.0.
# You may obtain a copy of MulanPSL - 2.0 at:
#
#   https://opensource.org/licenses/MulanPSL-2.0
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the MulanPSL - 2.0 for more details.
#

set -e

if [ -z "$MAPLE_ROOT" ]; then
  echo "Please \"source build/envsetup.sh\" to setup environment"
  exit 1
fi

today=$(date +%Y-%m-%d-%H_%M)

cd $MAPLE_ROOT
make clean

mkdir -p output
REPORT=output/report.txt
touch $REPORT

echo $today >> $REPORT

echo "start OpenArkCompiler build and testing..." >> $REPORT

echo "make setup" >> $REPORT
make setup
echo "make" >> $REPORT
make

echo "make libcore"
make libcore
echo "make testall" >> $REPORT
make testall 2>> $REPORT

cat $REPORT
echo "end OpenArkCompiler ..."
