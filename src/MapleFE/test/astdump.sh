#!/bin/bash
function usage {
cat << EOF

Usage: astdump.sh [-d] [-f] [-p <PREFIX>] [-a] [-c] [-k] [-A] [-C] [-n] [-t] <file1> [<file2> ...]

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
  -n | --name            Keep original names by removing "__lambda_[0-9]*__" and "__v[0-9]*" from generated code
  -t | --treediff        Compare the AST of generated TS code with the one of original TS code
  <file1> [<file2> ...]  Specify one or more TypeScript files to be processed
EOF
exit 1
}

DOT= PRE= LIST= VIEWOP= HIGHLIGHT="cat" TSCERR= KEEP= CLEAN= NAME= TREEDIFF=
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
        -t|--treediff)   TREEDIFF="--emit-ts-only"; NAME="original" ;;
        -*)              echo "Unknown option $1"; usage;;
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
    echo "$AST2CPP" "$ts".ast --trace=2 --emit-ts $TREEDIFF
    out=$("$AST2CPP" "$ts".ast --trace=2 --emit-ts $TREEDIFF 2>&1)
    [ $? -eq 0 ] || Failed="$Failed $ts"
  fi
  echo "$out"
  cmd=$(grep -n -e "^// .Beginning of Emitter:" -e "// End of Emitter.$" <<< "$out" |
    tail -2 | sed 's/:.*//' | xargs | sed 's/\([^ ]*\) \(.*\)/sed -n \1,$((\2))p/')
  if [ "x${cmd:0:4}" = "xsed " ]; then
    T=$ts-$$.out.ts
    eval $cmd <<< "$out" > "$T"
    [ -z "$NAME" ] || sed -i -e 's/__v[0-9][0-9]*//g' -e 's/ __lambda_[0-9][0-9]*__/ /' -e 's/function *\((.*) => \)/\1/' "$T"
    clang-format-10 -i --style="{ColumnLimit: 120}" "$T"
    echo -e "\n====== TS Reformatted ======\n"
    $HIGHLIGHT "$T"
    eval tsc -t es6 --lib es2015,es2017,dom -m commonjs --experimentalDecorators "$T" $TSCERR
    # --strict  --downlevelIteration --esModuleInterop --noImplicitAny --isolatedModules "$T" $TSCERR
    if [ $? -ne 0 ]; then
      E="tsc"
      grep -qm1 "PassNode *{" <<< "$out" && E="$E,PassNode"
      Failed="$Failed ($E)$ts"
      echo Failed to compile "$T" with tsc
      [ -n "$KEEP" ] || rm -f "$T"
    elif [ -z $TREEDIFF ]; then
      Passed="$Passed $ts"
      rm -f "$T"
    else
      clang-format-10 $ts > $ts.tmp.ts
      $TS2AST $ts.tmp.ts
      if [ $? -eq 0 ]; then
        $AST2CPP $ts.tmp.ts.ast $TREEDIFF | sed -n '/^AstDump:/,$p' | sed -e 's/LT_CharacterLiteral/LT_StringLiteral/' \
          -e 's/\(mStrIdx: unsigned int, \)[0-9]* =>/\1=>/'
      fi > $ts.orig
      ts2ast $T
      if [ $? -eq 0 ]; then
        $AST2CPP $T.ast $TREEDIFF | sed -n '/^AstDump:/,$p' | sed -e "s|$T|$ts.tmp.ts|" \
          -e 's/\(mStrIdx: unsigned int, \)[0-9]* =>/\1=>/'
      fi > $ts.gen
      diff $ts.orig $ts.gen
      if [ $? -eq 0 -a -s $ts.orig -a -s $ts.gen ]; then
        echo Passed with $ts
        Passed="$Passed $ts"
      else
        echo Failed to compare with the AST of $ts
        Failed="$Failed (ast)$ts"
      fi
      echo === "$T"; cat "$T"
      echo --- "$ts"; cat "$ts"
      echo ===
      if [ -n "$DOT" ]; then
        for tf in $ts.tmp.ts $T; do
          echo "=== $tf"
          cat $tf
          out="$out"$'\n'"$(ts2ast $tf --dump-dot)"
        done
      fi
      rm -f "$T" $ts.orig $ts.gen $ts.tmp.ts
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
