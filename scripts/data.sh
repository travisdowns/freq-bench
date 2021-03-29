#!/bin/bash
set -e

# This poorly named variable should be set to the
# TSC (time stamp counter) frequency of your system.
# You can determine this by running:
# ./bench dummy > /dev/null
# and looking at the 'tsc freq' line. Use that value
# as MHZ.
export MHZ=${MHZ:=3192}

# https://stackoverflow.com/a/12694189
SCRIPTDIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

. "$SCRIPTDIR/common.sh"

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

function run_one {
    export COLS=${2:-Cycles,Unhalt_GHz,tscg,retries}
    local test_name=$1
    echo ">>>>>>>>> Running $test_name with TEST_PER=$TEST_PER TEST_RES=$TEST_RES COLS=$COLS"
    if [[ $argstr && ! " $argstr " =~ " $test_name " ]]; then
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

# test that includes volts, we gate this behind "DO_VOLTS" because this will usually fail unless you
# are running as root, or have set up the msr dir for non-root access.
# for volts to work at all, you need to install msr-tools package, then it will work as root
# Then, here's how you set up for non-root msr reads:
# sudo chmod a+r /dev/cpu/*/msr
# sudo setcap cap_sys_rawio=ep ./bench
if [[ "$DO_VOLTS" -eq 1 ]]; then
    echo "Doing VOLTS data collection"
    TEST_RES=$((2 * $MHZ)) TEST_EXTRA=$extra NO_WARM=1 run_one vporxmm_vz100 "Cycles,Unhalt_GHz,IPC,volts" "-volts"
    TEST_RES=$((2 * $MHZ)) TEST_EXTRA=$extra NO_WARM=1 run_one vporymm_vz100 "Cycles,Unhalt_GHz,IPC,volts" "-volts"
    TEST_RES=$((2 * $MHZ)) TEST_EXTRA=$extra NO_WARM=1 run_one vporzmm_vz100 "Cycles,Unhalt_GHz,IPC,volts" "-volts"
else
    echo "Doing non-VOLTS data collection"
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
    TEST_EXTRA=$extra run_one vporxymm250          "Cycles,Unhalt_GHz,IPC"
    TEST_EXTRA=$extra run_one vporyzmm250          "Cycles,Unhalt_GHz,IPC"
    run_one vporymm
    run_one vporzmm

    for r in 1 2 3 4 5 6 7 8 9 10 20 30 40 50 60 70 80 90 100 120 140 160 180 200; do
        TEST_EXTRA=$extra run_one "vporxymm250_$r"            "Cycles,Unhalt_GHz"
    done
    TEST_EXTRA=$extra run_one "mulxymm250_10"            "Cycles,Unhalt_GHz"

    for p in $period_list; do
        TEST_EXTRA=$extra TEST_PER=$(($p * $MHZ)) run_one vporzmm_vz100 "Cycles,Unhalt_GHz,IPC" "-period$p"
    done
fi
