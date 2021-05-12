#!/bin/bash
function usage {
cat << EOF

Usage: astdump.sh [-dot] [-f|--fullscreen] [-p <PREFIX>|--pre <PREFIX>] [-a|--ast] [-c|--cfg] <file1> [<file2> ...]

  -d | --dot             Use Graphviz dot to generate the graph and view it with viewnior
  -f | --fullscreen      View the generated graph in fullscreen mode. It implies option -dot
  -p | --pre <PREFIX>    Filter graphs with the specified <PREFIX>, e.g. -p "CFG_<function-name>"
  -a | --ast             Show AST graph. It is equivalent to options "-dot -p AST"
  -c | --cfg             Show CFG graph. It is equivalent to options "-dot -p CFG"
  <file1> [<file2> ...]  Specify one or more TypeScript files to be processed
EOF
exit 1
}

DOT= PRE= LIST= VIEWOP=
while [ $# -gt 0 ]; do
    case $1 in
        -d|-dot|--dot)   DOT=true;;
        -f|--fullscreen) VIEWOP="--fullscreen"; DOT=true;;
        -p|-pre|--pre)   [ $# -ge 2 ] && { PRE="$2"; shift; } || { echo "$1 needs an argument"; exit 1; } ;;
        -a|-ast|--ast)   PRE="AST" ; DOT=true ;;
        -c|-cfg|--cfg)   PRE="CFG" ; DOT=true ;;
        -*)              usage;;
        *)               LIST="$LIST $1"
    esac
    shift
done
[ -n "$LIST" ] || { echo Please specify one or more TypeScript files.; usage; }
[ -z "$DOT" ] || [ -x /usr/bin/dot -a -x /usr/bin/viewnior ] || sudo apt install graphviz viewnior 
CMD=$(dirname $0)/../output/typescript/typescript/ts2cpp
[ -x "$CMD" ] || { echo Cannot execute $CMD; exit 1; }
Failed=
for ts in $LIST; do
  echo "$CMD" "$ts" --trace-a2c
  out=$("$CMD" "$ts" --trace-a2c 2>&1)
  [ $? -eq 0 ] || Failed="$Failed $ts"
  echo "$out"
  cmd=$(grep -n -e "^// .Beginning of AstEmitter:" -e "^// End of AstEmitter.$" <<< "$out" |
    tail -2 | sed 's/:.*//' | xargs | sed 's/\([^ ]*\) \(.*\)/sed -n \1,$((\2+1))p/')
  [ -n "$cmd" ] && { echo -e "\n====== Reformated ======\n"; eval $cmd <<<"$out" | clang-format-10; }
  grep -n -e "^digraph $PRE[^{]* {" -e "^}" <<< "$out" | grep -A1 "digraph [^{]* {" |
  if [ -n "$DOT" ]; then
    grep -n -e "^digraph $PRE[^{]* {" -e "^}" <<< "$out" | grep -A1 "digraph [^{]* {" |
      grep -v ^-- | sed 'N;s/\n/ /' | sed -e 's/:digraph [^{]* { */,/' -e 's/:.*/p/g' |
      while read cmd; do
        echo --- "$ts"; cat "$ts"
        sed -n $cmd <<< "$out" > "$ts".dot
        dot -Tpng -o "$ts".png "$ts".dot
        viewnior $VIEWOP "$ts".png
        rm -f "$ts".png "$ts".dot
      done
  fi
done
echo
[ -n "$Failed" ] || exit 0
echo "Test case(s) failed:"
echo $Failed | xargs -n1 | sort | nl
exit 1
