
good_dir=$1
bad_dir=$2
src_list=src_list.log
pwd=$PWD
cd $bad_dir
sed 's/ /\n/g' obj.list > $pwd/$src_list
cd -
file_lenth=`sed -n '$=' $src_list`

function get_src_list() {
  min=$1
  max=$2
  line_number=1
  src=" "
  cat $src_list | while read line
  do
  if [ "${line_number}" -ge "${min}" ]&&[ "${line_number}" -le "${max}" ];
  then
    src=${src}${bad_dir}"/"${line}" "
  else
    src=${src}${good_dir}"/"${line}" "
  fi
  if [ $line_number -eq $file_lenth ];then
    echo "${src}"
  fi
  line_number=`expr $line_number + 1`
  done
}

cur_max=`sed -n '$=' $src_list`
cur_min=1
last_max=$cur_max
while true
do
cur_src=`echo $(get_src_list $cur_min $cur_max)`
echo $file_lenth
bash run.sh $cur_src

if [ $? -eq 0 ];then

if [ $cur_min -eq $cur_max ];then
cur_min=$last_max
cur_max=$last_max
cur_src=`echo $(get_src_list $cur_min $cur_max)`
break
fi

cur_min=$cur_max
cur_max=${last_max}

else
if [ $cur_min -eq $cur_max ];then
cur_src=`echo $(get_src_list $cur_min $cur_max)`
break
fi
last_max=$cur_max
cur_max=$(((${cur_max}+${cur_min})/2))
fi
echo "==================================="
echo "cur_min" $cur_min
echo "cur_max" $cur_max
echo "==================================="

done

bash run.sh $cur_src
if [ $? -eq 0 ];then
  echo "bad src not found"
else
  echo "=================================="
  echo "bad src found:"
  head -$cur_min  $src_list | tail -1
  echo "=================================="
fi
