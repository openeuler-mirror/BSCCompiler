#!/bin/bash
bad_mpl=""
good_dir=""
bad_dir=""
candidate_type="object"
src_list=src_list.log
pwd=${PWD}
twd=${pwd}/temp_dir
LINARO=${MAPLE_ROOT}/tools/gcc-linaro-7.5.0
fileName=""

function parse_args() {
  while [ $# -ne 0 ]; do
      case "$1" in
          *.mpl)
              bad_mpl=$1
              candidate_type="mpl"
              fileName=${bad_mpl%\.mpl*}
              ;;
          *)
              if [ -d "$1" ]; then
                good_dir=$1
                shift
                bad_dir=$1
              else
                echo "no such directory ""$1"
                exit 1
              fi
              ;;
      esac
      shift
  done
}

function get_cur_func_list() {
  echo > ${twd}/temp_func.list
  min=$1
  max=$2
  line_number=1
  src=" "
  cat ${twd}/$src_list | while read line
  do
  if [ "${line_number}" -ge "${min}" ]&&[ "${line_number}" -le "${max}" ];
  then
    echo ${line} >> ${twd}/temp_func.list
  fi
  line_number=`expr $line_number + 1`
  done
}

function compileMpl() {
  #echo "cur parto2list:"
  #cat temp_func.list | lolcat
  $MAPLE_ROOT/output/aarch64-clang-release/bin/maple --partO2=${twd}/temp_func.list --run=me:mpl2mpl:mplcg --option="--O2 --quiet: --O2 --quiet: --O2 --quiet --no-pie --verbose-asm" ${twd}/$bad_mpl &> comb.log
  $LINARO/bin/aarch64-linux-gnu-gcc -O2 -std=c99 -o ${twd}/${fileName}.o -c ${twd}/${fileName}.s
}

function asmMpl() {
  cat temp_func.list
  python3 replace_func.py good.s bad.s ${twd}/temp_func.list > ${twd}/newgood.s
  $LINARO/bin/aarch64-linux-gnu-gcc -O2 -std=c99 -o ${twd}/${fileName}.o -c ${twd}/newgood.s
}

function main() {
  rm -rf ${twd}
  cp -r $good_dir ${twd}
  cp run.sh ${twd}/
  #cp replace_func.py ${twd}/
  #cp good.s ${twd}/
  #cp bad.s ${twd}/
  if [ "$candidate_type" == "mpl" ]; then
    grep -E "func &.*{$" $good_dir/$bad_mpl | awk '{print $2}' | sed 's/&//g' > ${twd}/$src_list
  else
    sed 's/ /\n/g' $good_dir/obj.list > $twd/$src_list
  fi

  obj_list=`sed 's/ /\n/g' $good_dir/obj.list`
  cd ${twd}
  file_length=`sed -n '$=' $src_list`

  cur_min=1
  cur_max=$file_length
  last_max=$cur_max
  while true
  do
    get_cur_func_list $cur_min $cur_max
    #echo $file_length

    echo "==================================="
    echo "cur_min" $cur_min
    echo "cur_max" $cur_max

    #asmMpl
    compileMpl
    bash run.sh $obj_list

    if [ $? -eq 0 ];then
      echo -e "\033[32mSUCCESS \033[0m"
      if [ $cur_min -eq $cur_max ];then
        cur_min=$last_max
        cur_max=$last_max
        get_cur_func_list $cur_min $cur_max
        break
      fi
      cur_min=$cur_max
      cur_max=${last_max}
    else
      echo -e "\033[31mFAILED \033[0m"
      if [ $cur_min -eq $cur_max ];then
        get_cur_func_list $cur_min $cur_max
        break
      fi
      last_max=$cur_max
      cur_max=$(((${cur_max}+${cur_min})/2))
    fi
    echo "==================================="
  done

  #asmMpl
  compileMpl
  echo "==================================="
  bash run.sh $obj_list
  if [ $? -eq 0 ];then
    echo "bad func not found"
  else
    echo "bad func found:"
    head -$cur_min  $src_list | tail -1
  fi
}

parse_args $@
main