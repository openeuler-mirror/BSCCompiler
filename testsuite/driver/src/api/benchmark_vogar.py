import os
import re
from api.shell_operator import ShellOperator


class BenchmarkVogar(ShellOperator):

    def __init__(self, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.command = ""
        self.vogar_deps_dir = "${MAPLE_ROOT}/zeiss/prebuilt/tools/mygote_script/benchmark_scripts"
        self.sourcepath = ""

    def get_command(self, variables):
        if "SOURCEPATH" in variables:
            self.sourcepath = "--sourcepath " + variables["SOURCEPATH"]
        self.command = "PATH=${MAPLE_ROOT}/../out/soong/host/linux-x86/bin:/$PATH ANDROID_BUILD_TOP=${MAPLE_ROOT}/.. VOGAR_DEPS_DIR=" + self.vogar_deps_dir + " java -classpath " + self.vogar_deps_dir + "/vogar.jar vogar.Vogar --results-dir . --toolchain D8 --mode DEVICE --variant X64 " + self.sourcepath + " --benchmark " + variables["BENCHMARK_CASE"]
        return super().get_final_command(variables)