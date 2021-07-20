#!/bin/bash
function usage {
cat << EOF

Usage: astdump.sh [-d] [-f] [-p <PREFIX>] [-a] [-c] [-k] [-A] [-C] [-n] <file1> [<file2> ...]

Short/long options:
  -d | --dot             Use Graphviz dot to generate the graph and view it with viewnior
  -f | --fullscreen      View the generated graph in fullscreen mode. It implies option -dot
  -p | --pre <PREFIX>    Filter graphs with the specified <PREFIX>, e.g. -p "CFG_<function-name>"
  -a | --ast             Show AST graph. It is equivalent to options "-d -p AST"
  -c | --cfg             Show CFG graph. It is equivalent to options "-d -p CFG"
  -s | --syntax          Syntax highlighting the generated TypeScript code
  -k | --keep            Keep generated files *.ts-[0-9]*.out.ts which fail to compile with tsc
  -A | --all             Process all .ts files in current directory excluding *.ts-[0-9]*.out.ts
  -C | --clean           Clean up generated files (*.ts-[0-9]*.out.ts)
  -n | --name            Keep original names by removing suffixes "__v[0-9]*" from generated code
  <file1> [<file2> ...]  Specify one or more TypeScript files to be processed
EOF
exit 1
}

DOT= PRE= LIST= VIEWOP= HIGHLIGHT="cat" TSCERR= KEEP= CLEAN= NAME=
while [ $# -gt 0 ]; do
    case $1 in
        -d|--dot)        DOT=dot;;
        -f|--fullscreen) VIEWOP="--fullscreen"; DOT=fullscreen;;
        -p|--pre)        [ $# -ge 2 ] && { PRE="$2"; shift; } || { echo "$1 needs an argument"; exit 1; } ;;
        -a|--ast)        PRE="AST" ; DOT=ast ; TSCERR=">& /dev/null" ;;
        -c|--cfg)        PRE="CFG" ; DOT=cfg ; TSCERR=">& /dev/null" ;;
        -s|--syntax)     HIGHLIGHT="highlight -O xterm256 --syntax ts" ;;
        -e|--tscerror)   TSCERR= ;;
        -k|--keep)       KEEP=keep ;;
        -C|--clean)      CLEAN=clean ;;
        -A|--all)        LIST="$LIST $(find -maxdepth 1 -name '*.ts' | grep -v '\.ts-[0-9][0-9]*\.out.ts')" ;;
        -n|--name)       NAME="original" ;;
        -*)              usage;;
        *)               LIST="$LIST $1"
    esac
    shift
done
if [ -n "$CLEAN" ]; then
  echo Cleaning up generated files...
  find -maxdepth 1 -regex '.*\.ts-[0-9]+\.out.[ctj][ps]p*\|.*\.ts-[0-9]+\.[pd][no][gt]\|.*\.ts.[ca][ps][pt]' -exec rm '{}' \;
  echo Done.
fi
[ -n "$LIST" ] || { echo Please specify one or more TypeScript files.; usage; }
[ -z "$DOT" ] || [ -x /usr/bin/dot -a -x /usr/bin/viewnior -a -x /usr/bin/highlight ] || sudo apt install graphviz viewnior highlight
TSOUT=$(cd $(dirname $0)/../; pwd)/output/typescript
TS2AST=$TSOUT/bin/ts2ast
AST2CPP=$TSOUT/bin/ast2cpp
[ -x "$TS2AST" ] || { echo Cannot execute $TS2AST; exit 1; }
[ -x "$AST2CPP" ] || { echo Cannot execute $AST2CPP; exit 1; }
Passed=
Failed=
for ts in $LIST; do
  echo ---------
  echo "$TS2AST" "$ts"
  out=$("$TS2AST" "$ts")
  if [ $? -ne 0 ]; then
    Failed="$Failed $ts"
  else
    echo "$AST2CPP" "$ts".ast --trace-a2c
    out=$("$AST2CPP" "$ts".ast --trace-a2c 2>&1)
    [ $? -eq 0 ] || Failed="$Failed $ts"
  fi
  echo "$out"
  cmd=$(grep -n -e "^// .Beginning of Emitter:" -e "// End of Emitter.$" <<< "$out" |
    tail -2 | sed 's/:.*//' | xargs | sed 's/\([^ ]*\) \(.*\)/sed -n \1,$((\2))p/')
  if [ "x${cmd:0:4}" = "xsed " ]; then
    T=$ts-$$.out.ts
    eval $cmd <<< "$out" > "$T"
    [ -z "$NAME" ] || sed -i 's/__v[0-9][0-9]*//g' "$T"
    clang-format-10 -i --style="{ColumnLimit: 120}" "$T"
    echo -e "\n====== TS Reformated ======\n"
    $HIGHLIGHT "$T"
    eval tsc -t es6 --lib es2015,es2017,dom -m commonjs --experimentalDecorators "$T" $TSCERR
    # --strict  --downlevelIteration --esModuleInterop --noImplicitAny --isolatedModules "$T" $TSCERR
    if [ $? -ne 0 ]; then
      E="tsc"
      grep -qm1 "PassNode *{" <<< "$out" && E="$E,PassNode"
      Failed="$Failed ($E)$ts"
      echo Failed to compile "$T" with tsc
      [ -n "$KEEP" ] || rm -f "$T"
    else
      Passed="$Passed $ts"
      rm -f "$T"
    fi
  fi
  if [ -n "$DOT" ]; then
    echo --- "$ts"; cat "$ts"
    idx=0
    grep -n -e "^digraph $PRE[^{]* {" -e "^}" <<< "$out" | grep -A1 "digraph [^{]* {" |
      grep -v ^-- | sed 'N;s/\n/ /' | sed -e 's/:digraph [^{]* { */,/' -e 's/:.*/p/g' |
      { while read cmd; do
        idx=$((idx+1))
        sed -n $cmd <<< "$out" > "$ts"-$idx.dot
        dot -Tpng -o "$ts"-$idx.png "$ts"-$idx.dot
        env LC_ALL=C viewnior $VIEWOP "$ts"-$idx.png &
      done
      wait
      rm -f "$ts"-[0-9]*.png "$ts"-[0-9]*.dot; }
  fi
done
echo
echo "Test case(s) passed:"
echo $Passed | xargs -n1 | env LC_ALL=C sort | nl
echo
[ -n "$Failed" ] || exit 0
echo "Test case(s) failed:"
echo $Failed | xargs -n1 | env LC_ALL=C sort | sed 's/)/) /' | nl
exit 1
