#!/bin/bash
set -e

# configure the following values for your system
export MHZ=${MHZ:=3192}

# https://stackoverflow.com/a/12694189
SCRIPTDIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

RESULTDIR="$SCRIPTDIR/../results"
: ${PREFIX:=test}

: ${TEMPDIR:=$SCRIPTDIR/../tmp}

mkdir -p "$RESULTDIR"
mkdir -p "$TEMPDIR"

echo "Collecting data and writing to $RESULTDIR/$PREFIX-*.csv"

PER_US=5000

export TEST_PER=$(($PER_US * $MHZ))
export TEST_RES=$((1 * $MHZ))
extra=$((100 * $MHZ)) # 100 us

argstr="$@"

function run_one {
    export COLS=${2:-Cycles,Unhalt_GHz,tscg,retries}
    local test_name=$1
    echo "TEST_PER=$TEST_PER TEST_RES=$TEST_RES COLS=$COLS"
    if [[ $argstr && ! " $argstr " =~ " $1 " ]]; then
        echo "Skipping $1 because it is not in $argstr"
        return
    fi
    if [[ -n $SKIP_ZMM && $test_name =~ .*zmm.* ]]; then
        echo "Skipping $test_name because SKIP_ZMM is set"
        return
    fi
    ./bench $test_name > "$TEMPDIR/temp.csv"
    for i in {0..2}; do
        egrep -B1 "^${i}," "$TEMPDIR/temp.csv" > "$RESULTDIR/$PREFIX-$test_name${3}-${i}.csv"
    done
}

echo "argstr: $argstr"

run_one vporxmm_vz
run_one vporymm_vz
run_one vporzmm_vz
TEST_EXTRA=$extra run_one vporxmm_vz100        "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra run_one vporymm_vz100        "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra run_one vporzmm_vz100        "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra TEST_RES=$((8 * $MHZ)) run_one vporzmm_vz100        "Cycles,Unhalt_GHz,IPC" "-8us"
TEST_EXTRA=$extra run_one vporxmm_tput_vz100   "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra run_one vporymm_tput_vz100   "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra run_one vporzmm_tput_vz100   "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra run_one vpermdzmm_vz100      "Cycles,Unhalt_GHz,IPC"
TEST_EXTRA=$extra run_one vpermdzmm_tput_vz100 "Cycles,Unhalt_GHz,IPC"
run_one vporymm
run_one vporzmm

