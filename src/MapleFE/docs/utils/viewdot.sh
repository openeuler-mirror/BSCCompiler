#!/bin/bash
if [ $# -lt 1 ]; then
  out=$(cat)
  tmpdir=$(mktemp -dt viewdot-XXXXXX)
  trap "rm -rf $tmpdir" SIGINT SIGQUIT SIGKILL
  grep -n -e "digraph [^{]* {" -e "^} // digraph JS" <<< "$out" | grep -A1 "digraph [^{]* {" |
    grep -v ^-- | sed 'N;s/\n/ /' | sed -e 's/:.*digraph [^{]* { */,/' -e 's/:.*/p/g' |
      { while read cmd; do
        name=$(sed -n $cmd <<< "$out" | head -1 | sed 's/.*digraph \([^{]*\) {.*/\1/')
        echo $$-$name
        sed -n $cmd <<< "$out" > "$tmpdir"/$$-$name.dot
        dot -Tpng -o "$tmpdir"/$$-$name.png "$tmpdir"/$$-$name.dot
        env LC_ALL=C viewnior "$tmpdir"/$$-$name.png &
      done
      wait
      rm -rf "$tmpdir"; }
  exit
fi
for f; do
  dot -Tpng -o $f.png $f
  viewnior $f.png
  rm -f $f.png
done
