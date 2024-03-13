#!/bin/bash

FILE=""
CMD=""
PID=""
SORT=false

function usage() {
    echo "USAGE:"
    echo "  ./handle-ps.sh -f <filename> [-s] [-q <CMD>] [-p <PID>]"
    echo "OPTIONS:"
    echo "  -f: input file"
    echo "  -s: sort by priority and pid"
    echo "  -q: query processes with queried command"
    echo "  -p: get process's ancestors"
}

while getopts "f:q:p:sh" opt; do
    case $opt in
        f) FILE=${OPTARG} ;;
        q) CMD=${OPTARG} ;;
        p) PID=${OPTARG} ;;
        s) SORT=true ;;
        h) usage; exit 0 ;;
        ?) usage; exit 1 ;;
    esac
done

if [ -z $FILE ]; then
    usage
    exit 1
fi

# You can remove ":" after finishing.
if $SORT; then
    # Your code here. (1/3)
    sort -k4nr -k2n $FILE

elif [ ! -z "$CMD" ]; then
    # Your code here. (2/3)
    cmd='$5 ~ /'$CMD'/ {print $0}'
    # echo $cmd
    awk "$cmd"  $FILE
elif [ ! -z $PID ]; then
    # Your code here. (3/3)
    awk -v input=$PID '
    {
	tgt = input
	flag = 1
	while(flag) {
		flag=0
		if($2==tgt) {
			print $3
			tgt=$3
			flag=1
			NF=0
		}
	}
    }' $FILE
else
    usage
    exit 1
fi
