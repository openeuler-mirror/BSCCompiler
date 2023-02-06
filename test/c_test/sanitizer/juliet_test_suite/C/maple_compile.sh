#!/bin/bash

OPT_LEVEL="$1"
FNO_INLINE='--no-inline'
SAN="--san=$2"

src=$3

cmd="maple --save-temps --run=me:mpl2mpl:mplcg --option='$OPT_LEVEL $SAN:$OPT_LEVEL $FNO_INLINE -quiet:-O1' $src"
eval $cmd
