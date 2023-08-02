#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

from api import *

TIME = {
    "compile": [
        Shell(
            "OUTDIR=output;"
            "mkdir -p ${OUTDIR}"
        ),
        Shell(
            "num=1;"
            "TIMES=5;"
            "cpu_list=(0 1 2 3 4 5 6 7);"
            "cpu_governor=\'userspace\';"
            "offline_cpu_list=(4 5 6 7);"
            "enable_cpu_list=(0 1 2 3);"
            "cpu_freq=1805000"
        ),
        Shell(
            "for offline_cpu in ${offline_cpu_list[@]}; do"
            "  adb shell \"echo 0 > /sys/devices/system/cpu/cpu${offline_cpu}/online\";"
            "done"
        ),
        Shell(
            "for enable_cpu in ${enable_cpu_list[@]}; do"
            "  adb shell \"echo $cpu_freq > /sys/devices/system/cpu/cpu${enable_cpu}/cpufreq/scaling_max_freq\";"
            "  adb shell \"echo $cpu_freq > /sys/devices/system/cpu/cpu${enable_cpu}/cpufreq/scaling_min_freq\";"
            "  adb shell \"echo $cpu_governor > /sys/devices/system/cpu/cpu${enable_cpu}/cpufreq/scaling_governor\";"
            "  adb shell \"echo $cpu_freq > /sys/devices/system/cpu/cpu${enable_cpu}/cpufreq/scaling_setspeed\";"
            "done"
        ),
        Shell(
            "adb shell stop;"
            "sleep 20s"
        ),
        Shell(
            "while true; do"
            "  if [ ${num} -gt ${TIMES} ]; then"
            "    break;"
            "  fi"
        ),
        Shell(
            "adb shell \"mkdir -p /data/maple/${CASE}/${OPT}\""
        ),
        Shell(
            "adb push ${APP}.dex /data/maple/${CASE}/${OPT}/"
        ),
        Shell(
            "adb shell \"(time /data/maple/maple --gconly -O2 --no-with-ipa /data/maple/${CASE}/${OPT}/${APP}.dex --hir2mpl-opt=\\\"-dump-time -no-mpl-file -Xbootclasspath /data/maple/core-oj.jar,/data/maple/core-libart.jar,/data/maple/bouncycastle.jar,/data/maple/apache-xml.jar,/data/maple/framework.jar,/data/maple/ext.jar,/data/maple/telephony-common.jar,/data/maple/voip-common.jar,/data/maple/ims-common.jar,/data/maple/android.test.base.jar,/data/maple/featurelayer-widget.jar,/data/maple/hwEmui.jar,/data/maple/hwPartBasicplatform.jar,/data/maple/telephony-separated.jar,/data/maple/hwTelephony-common.jar,/data/maple/hwPartTelephony.jar,/data/maple/hwPartTelephonyVSim.jar,/data/maple/hwPartTelephonyCust.jar,/data/maple/hwPartTelephonyTimezoneOpt.jar,/data/maple/hwPartTelephonyOpt.jar,/data/maple/hwPartSecurity.jar,/data/maple/hwIms-common.jar,/data/maple/hwPartMedia.jar,/data/maple/hwPartConnectivity.jar,/data/maple/hwPartPowerOffice.jar,/data/maple/hwPartDeviceVirtualization.jar,/data/maple/hwPartAirSharing.jar,/data/maple/hwPartDefaultDFR.jar,/data/maple/hwPartDFR.jar,/data/maple/hwPartMagicWindow.jar,/data/maple/hwframework.jar,/data/maple/com.huawei.nfc.jar,/data/maple/org.ifaa.android.manager.jar,/data/maple/hwaps.jar,/data/maple/servicehost.jar,/data/maple/hwcustIms-common.jar,/data/maple/hwcustTelephony-common.jar,/data/maple/hwIAwareAL.jar,/data/maple/conscrypt.jar,/data/maple/updatable-media.jar,/data/maple/okhttp.jar --java-staticfield-name=smart\\\" --mplcg-opt=\\\"--no-ico --no-cfgo --no-prepeep --no-ebo --no-storeloadopt --no-globalopt --no-schedule --no-proepilogue --no-peep --no-const-fold --no-lsra-hole  --with-ra-linear-scan --no-prelsra --no-prespill --no-lsra-hole\\\" -time-phases)\" >& ${OUTDIR}/maple_${APP}_${num}.txt &"
        ),
        Shell(
            "count=1;"
            "mkdir -p ${OUTDIR}/mem_out;"
            "while true; do"
            "  pid=`adb shell pidof maple`;"
            "  if [[ -z ${pid} ]]; then"
            "    echo \"compile ${APP} ${num} complete\";"
            "    break;"
            "  fi;"
            "  adb shell showmap ${pid} >> ${OUTDIR}/mem_out/mem_${count}.log;"
            "  ((count++));"
            "  sleep 0.5;"
            "done"
        ),
        Shell(
            "wait;"
            "file_list=`ls ${OUTDIR}/mem_out | grep log | uniq`;"
            "PSSMAX=0;"
            "for file in ${file_list}; do"
            "  pss=`cat ${OUTDIR}/mem_out/$file | grep TOTAL | awk '{print $3+$9}'`;"
            "  if [[ ${pss} -ge ${PSSMAX} ]]; then"
            "    PSSMAX=${pss};"
            "  fi;"
            "done"
        ),
        Shell(
            "echo \"${PSSMAX}\" >> ${OUTDIR}/pss_max_${APP}.txt;"
            "rm -rf ${OUTDIR}/mem_out"
        ),
        Shell(
            "  adb shell \"rm -rf /data/maple/${CASE}/${OPT}\";"
            "  num=`expr ${num} + 1`;"
            "done" #end while
        ),
        Shell(
            "adb shell start"
        )
    ],
    "checktime": [
        Shell(
            "python3 ${OUT_ROOT}/target/product/public/bin/checker_compiler_time_ci.py -d ${APP} -n 5 -t ${CHECKTIME} -o ${OUTDIR}" #${TIMES}
        )
    ]
}
