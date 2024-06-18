#!/usr/bin/env bash

filename=${1:-""}
writestring=${2:-""}

[ $# -lt 2 ] && echo "Invalid arguments $0: filename string" && exit 1

dir=${filename%\/*}
#clean the file if it doesn't exist otherwise create new one
[ ! -d $dir ] && mkdir -p $dir
[ ! -f $filename ] && touch $filename
echo "$writestring" > $filename
