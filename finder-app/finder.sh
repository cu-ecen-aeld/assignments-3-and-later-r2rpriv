#!/usr/bin/env bash

filesdir=${1:-""}
searchstr=${2:-""}

[ $# -lt 2 ] && echo "Invalid command: ${0##*\/}: filesdir searchstring" && exit 1

[ ! -d "$filesdir" ] && echo "Directory "$filesdir" not found" && exit 1

X=$(grep -Ril $searchstr $filesdir | wc -l)
Y=$(grep -Ri $searchstr $filesdir | wc -l)

echo "The number of files are $X and the number of matching lines are $Y"
