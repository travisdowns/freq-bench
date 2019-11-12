#! /bin/echo dont-run-this-directly

set -e
SCRIPTNAME=$(basename "$0")
SCRIPTDIR=$(dirname "$0")

# call this from the parent directory, not from inside scripts
# use it like script.sh [OUTPUT]
# where OUTPUT is the output filename for the plot, or not filename for an interactive graph
if [ ! -f "$PWD/scripts/$SCRIPTNAME" ]; then
    set +e
    PARENT="$( cd "$(dirname "$0")/.." ; pwd -P )"
    echo "Please run this script from the root project directory: $PARENT"
    exit 1
fi

: ${START:=1}
: ${STOP:=100000}

if [ -z "$OUTFILE" ]; then
    OUT=()
else
    OUT=("--out" "$OUTFILE")
fi

export PLOT=1

mkdir -p tmp
TDIR=$(mktemp -d "./tmp/XXXXXXXX")

MICROCODE=$(grep -m1 microcode /proc/cpuinfo | grep -o '0x.*')
NEWLINE=$'\n'

PLOT1=./scripts/plot1.sh
PLOT2=./scripts/plot2.sh
PLOT3=./scripts/plot3.sh
PLOT4=./scripts/plot4.sh
PLOTPY=./scripts/plot-csv.py

# arrayify values passed as strings from parent
COLARRAY=($COLS)
C2ARRAY=($COLS2)
#IFS=',' read -r -a arr <<< "$COUNTER_LIST"

COMMON_ARGS=(-v --xcol 0\
    ${COLS:+--cols ${COLARRAY[@]}} \
    ${COLS2:+--cols2 ${C2ARRAY[@]}} \
    ${CLABELS:+--clabels "$CLABELS"} \
    ${YLABEL:+--ylabel "$YLABEL"}
    --title "$TITLE" \
    "${OUT[@]}")

