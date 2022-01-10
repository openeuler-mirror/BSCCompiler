#!/bin/bash
# Usage: cd MapleFE/test/typescript/unit_tests; ../ts2cpp-test.sh *.ts
[ $# -lt 1 ] && exec $0 $(git ls-files "*.ts")
SUCC=
MPLFEPATH=$(cd $(dirname $0)/../../; pwd)
TSOUT=$MPLFEPATH/output/typescript
RTSRC=$MPLFEPATH/ast2cpp/runtime/src
RTINC=$MPLFEPATH/ast2cpp/runtime/include
ASTINC=$MPLFEPATH/astopt/include
TS2AST=$TSOUT/bin/ts2ast
AST2CPP=$TSOUT/bin/ast2cpp
TSCSH=$(dirname $0)/tsc.sh

# Acquire/release a lock
typeset -i LockVar
LockVar=1
function AcquireLock {
    while [[ $LockVar -ne 0 ]] || sleep 0.1; do
        ln -s Lock_$2 $1-lock-$((LockVar=(LockVar+1)%$3)) > /dev/null 2>&1 && break
    done
}
function ReleaseLock {
    rm -f $1-lock-$LockVar
}
trap "{ pstree -p $$ | tr ')' '\n' | sed 's/.*(//' | xargs kill -9 2> /dev/null; rm -f ts2cpp-lock-*; }" SIGINT SIGQUIT SIGKILL SIGTERM
rm -rf ts2cpp-lock-* *-ts2cpp.out ts2cpp.summary.out ts2cpp.failures*.out
cnt=0
if [ $# -gt 1 ]; then
  list1=$(grep -L -e "^ *import " -e "^ *export .* from " "$@")
  list2=$(grep -l -e "^ *import " -e "^ *export .* from " "$@")
else
  list1="$@" list2=
fi
single="no"
for list in "$list1" "$list2"; do
for f in $list; do
  echo $((++cnt)): $f
  t=$(basename $f .ts)
  [ -f $t.ts ] && f=$t.ts
  AcquireLock ts2cpp for_$t $(nproc)
  (set -x
  while true; do
    $TS2AST $f || { echo "(ts2ast)$f" >> ts2cpp.failures.out; break; }
    dep=$(sed 's/[ ,:|]\(import(\)/\n\1/g' "$f" | grep -E "^ *[ei][xm]port.*( from |require|\( *['\"])" | \
      sed -r "s/^ *[ei][xm]port.*( from |require *\(|\() *['\"]([^'\"]*).*/\2.cpp/" | sort -u)
    for cpp in $dep; do
      ts=$(sed 's/\.cpp/.ts/' <<< "$cpp")
      $TS2AST $ts
      dep="$dep "$(sed 's/[ ,:|]\(import(\)/\n\1/g' "$ts" | grep -E "^ *[ei][xm]port.*( from |require|\( *['\"])" | \
        sed -r "s/^ *[ei][xm]port.*( from |require *\(|\() *['\"]([^'\"]*).*/\2.cpp/" | sort -u)
    done
    dep=$(echo $dep | xargs -n1 | sort -u)
    $AST2CPP $f.ast || { echo "(ast2cpp)$f" >> ts2cpp.failures.out; break; }
    g++ -std=c++17 -g -I$RTINC -I$ASTINC $t.cpp $RTSRC/*.cpp $dep -o $t.out || { echo "(g++)$f" >> ts2cpp.failures2.out; break; }
    ./$t.out 2>&1 > $f-run.out || { echo "(run)$f" >> ts2cpp.failures2.out; break; }
    $TSCSH $f
    diff $f-run.out $f-nodejs.out
    if [ $? -ne 0 ]; then
       sed -e 's/^[A-Za-z]* {/{/' $f-run.out | diff - $f-nodejs.out
       if [ $? -ne 0 ]; then
         sed -e 's/^[A-Za-z]* {/{/' -e 's/} [A-Za-z]* {/} {/' $f-run.out | diff - $f-nodejs.out
         if [ $? -ne 0 ]; then
           echo "(result)$f" >> ts2cpp.failures3.out
           break
         fi
       fi
    fi
    echo $t >> ts2cpp.summary.out
    break
  done
  ReleaseLock ts2cpp
  ) >& $f-ts2cpp.out &
  if [ $single = "yes" ]; then
    wait
  fi
done 2>&1 
wait
single="yes"
done
num=$(echo $list1 $list2 | wc -w)
total=$(git ls-files "*.ts" | wc -w)
log=cxx-tmp.log
[ $num -eq $total ] && log=cxx.log
if [ -f ts2cpp.summary.out ]; then
  echo -e "\nDate: $(date)\nTest cases passed:" | tee -a $log
  sort ts2cpp.summary.out | xargs -n1 | nl | tee -a $log
fi
if [ -f ts2cpp.failures3.out ]; then
  echo -e "\nTest cases failed due to unexpected results:" | tee -a $log
  sort ts2cpp.failures3.out | xargs -n1 | nl | tee -a $log
  if [ $num -eq 1 ]; then
    echo -e  "\ndiff $t.ts-nodejs.out $t.ts-run.out"
    diff $t.ts-nodejs.out $t.ts-run.out
  fi
fi
if [ -f ts2cpp.failures2.out ]; then
  echo -e "\nTest cases failed due to g++ or run:" | tee -a $log
  sort ts2cpp.failures2.out | xargs -n1 | nl | tee -a $log
  if [ $num -eq 1 ]; then
    echo -e  "\nCommand line to compile $t.cpp:"
    grep -- "-std=c++17" $t.ts-ts2cpp.out | sed 's/^+/ /'
  fi
fi
if [ -f ts2cpp.failures.out ]; then
  echo -e "\nTest cases failed due to ts2ast or ast2cpp:" | tee -a $log
  sort ts2cpp.failures.out | xargs -n1 | nl | tee -a $log
fi
if [ $num -eq $total ]; then
  grep -c ": error:" *.ts-ts2cpp.out | sort -nrt: -k2 | grep -v :0 | sed 's/-ts2cpp.out//' > cxx-error.log
  lines=$(grep -n -e "Test cases passed:" -e "Test cases failed due to g++ or run:" $log | \
    grep -A1 ":Test cases passed:" | tail -2 | cut -d: -f1)
  sed -n $(echo $lines | sed 's/[^0-9]/,/')p $log | grep "[0-9]" | expand | cut -c9- > cxx-succ.log
else
  echo Saved testing results to file $log
fi
