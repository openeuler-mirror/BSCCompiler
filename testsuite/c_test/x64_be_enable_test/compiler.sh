#!/bin/bash

check_error()
{
  error_no=$?
  if [ $error_no != 0 ];then
    exit $error_no
  fi
}

init_env()
{
  if [ -f multiprocess.sh ]; then
    source multiprocess.sh
    MultiProcessInit
  fi
}

check_env()
{
  if [ ! -n ${MAPLE_ROOT} ];then
     echo "plz cd to maple root dir and source build/envsetup.sh arm release/debug"
     exit -2
  fi

  if [ ! -f ${BIN_MPLFE} ] || [ ! -f ${BIN_MAPLE} ];then
     echo "plz make to ${MAPLE_ROOT} and make tool chain first!!!"
     exit -3
  fi
}

init_parameter()
{
 OUT="a.out"
 LINK_OPTIONS=
 BIN_MPLFE=${MAPLE_BUILD_OUTPUT}/bin/hir2mpl
 BIN_MAPLE=${MAPLE_BUILD_OUTPUT}/bin/maple
 BIN_CLANG=${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang
 OPT_LEVEL="-O0"
 DEBUG="false"
 DEBUG_OPTION=
 CFLAGS=
}

prepare_options()
{
 if [ "${DEBUG}" == "true" ]; then
   # inline may delete func node and didn't update debug info, it'll cause debug info error when linking
   DEBUG_OPTION="-g --mpl2mpl-opt=--skip-phase=inline"
 fi
}

# $1 cfilePath
# $2 cfileName
generate_ast()
{
  set -x
  $BIN_CLANG ${CFLAGS} -I ${MAPLE_BUILD_OUTPUT}/lib/include -I ${OUT_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/lib/clang/12.0.0/include  --target=x86_64 -U __SIZEOF_INT128__ -emit-ast $1 -o "$2.ast"
  check_error
  set +x
}

# $1 cfileName
generate_mpl()
{
  set -x
  ${BIN_MPLFE} "$1.ast" -o "${1}_mplfe.mpl"
  check_error
  set +x
}

# $1 cfileName
generate_s()
{
  set -x
  ${BIN_MAPLE} ${OPT_LEVEL} ${DEBUG_OPTION} --mplcg-opt="--verbose-asm --verbose-cg" --genVtableImpl --save-temps "${1}_mplfe.mpl"
  check_error
  set +x
}

# $@ sFilePaths
link()
{
  set -x
  $BIN_CLANG ${OPT_LEVEL} ${LINK_OPTIONS} -o $OUT $@
  check_error
  set +x
}

help()
{
  echo
  echo "USAGE"
  echo "    ./compile.sh [options=...] files..."
  echo
  echo "EXAMPLE"
  echo "    ./compile.sh out=test.out ldflags=\"-lm -pthread\" test1.c test2.c"
  echo
  echo "OPTIONS"
  echo "    out:           binary output path, default is a.out"
  echo "    ldflags:       ldflags"
  echo "    optlevel:      -O0 -O1 -O2(default)"
  echo "    debug:         true(-g), false(default)"
  echo "    cflags:        cflags"
  echo "    help:          print help"
  echo
  exit -1
}

parse_cmdline()
{
 while [ -n "$1" ]
 do
   OPTIONS=`echo "$1" | sed 's/\(.*\)=\(.*\)/\1/'`
   PARAM=`echo "$1" | sed 's/.*=//'`
   case "$OPTIONS" in
   out) OUT=$PARAM;;
   ldflags) LINK_OPTIONS=$PARAM;;
   optlevel) OPT_LEVEL=$PARAM;;
   debug) DEBUG=$PARAM;;
   cflags) CFLAGS=$PARAM;;
   help) help;;
   *) if [ `echo "$1" | sed -n '/.*=/p'` ];then
        echo "Error!!! the parttern \"$OPTIONS=$PARAM\" can not be recognized!!!"
        help;
      fi
      break;;
   esac
   shift
 done
 files=$@
 if [ ! -n "$files" ];then
    help
 fi
}

main()
{
 init_env
 init_parameter
 check_env
 parse_cmdline $@
 prepare_options
 s_files=`echo ${files}|sed 's\\.c$\_mplfe.s\g'`
 for i in $files
 do
   fileName=${i//.c/}
   generate_ast $i $fileName
   generate_mpl $fileName
   generate_s $fileName
 done
 link $s_files
}

main $@ 2>&1 | tee log.txt
