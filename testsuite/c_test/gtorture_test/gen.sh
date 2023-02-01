#! /bin/bash

FILE_FROM=/home/anrui/The3rdDistrict/maple-ops/ctorture
PROG_DIR=`pwd`
COUNT=1

for file in `cat $FILE_FROM/work.list`
do
  if [[ $file == execute* ]]; then
     FILETAG=${file##*/}
     FILENAME=$(echo $FILETAG | awk -F '.' '{print $1}')
     HEAD=`printf "%05d\n" $COUNT`
     CASENAME=GCC$HEAD-g.torture.execute-$FILENAME
     mkdir -p $PROG_DIR/$CASENAME
     pushd $PROG_DIR/$CASENAME > /dev/null
     cp $FILE_FROM/$file .
     gcc $FILETAG > error.log
     if [ $? == 0 ]; then
       rm error.log
     fi
     ./a.out > expected.txt
     touch test.cfg
     echo "clean()" > test.cfg
     echo "compile($FILENAME)" >> test.cfg
     echo "run($FILENAME)" >> test.cfg

     COUNT=`expr $COUNT + 1`
  fi
done
