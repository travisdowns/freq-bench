#!/bin/bash
set -e

# https://stackoverflow.com/a/12694189
SCRIPTDIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

. "$SCRIPTDIR/common.sh"

PLOTPY="$SCRIPTDIR/plot-csv.py"
RESULTDIR="$SCRIPTDIR/../results"

# $1 input file(s)
# $2 output file (empty for default)
# $3 ylabel
# $4 xlabel
# $5 title
function plot {
    local title=$5
    if [[ $argstr && ! "$title" =~ "$argstr" ]]; then
        echo "Skipping $title because it is not in $argstr"
        return
    fi
    eval input=( $1 )
    IFS=',' command eval 'echo input="${input[*]}"'
    if [ -z "$OUTDIR" ]; then
        local OUT=()
    else
        mkdir -p "$OUTDIR"
        if [ -z "$2" ]; then
            local OUTNAME=${1%.*}.svg
        else
            local OUTNAME=$2.svg
        fi
        local OUT=("--out" "$OUTDIR/$OUTNAME")
    fi
    input=(${input[@]/#/"$RESULTDIR/"})
    # IFS=',' command eval 'echo input="${input[*]}"'
    echo "INPUT: ${input[@]} OUTPUT: $OUTNAME"
    "$PLOTPY" "${input[@]}" "${OUT[@]}" --tight --ylabel "$3" --xlabel "$4" --title "$title" "${@:6}"
}

: ${PREFIX:=skx}

# scatter_args="--scatter --markersize=1"
xcols_arg="--xcols-by-name us,us_1,us_2"
ycols_arg="--cols-by-name Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2"

plot "$PREFIX-vporymm_vz-{0..2}.csv" "fig-vporvz256" "Frequency (GHz)" "Time (us)" "256-bit VPOR Frequency Transitions" \
   $xcols_arg $ycols_arg --ylim 0 4 $scatter_args

plot "$PREFIX-vporzmm_vz-{0..2}.csv" "fig-vporvz512" "Frequency (GHz)" "Time (us)" "512-bit VPOR Frequency Transitions" \
   $xcols_arg $ycols_arg --ylim 0 4 $scatter_args

plot "$PREFIX-vporzmm_vz-{0..2}.csv" "fig-vpor-zoomed" "Frequency (GHz)" "Time (us)" "Transition Detail" --marker=. \
   --patches \
'[{ "xy" : [15000, 0],  "width" : 9,"height" : 4,"color" : "thistle"},'\
'{ "xy" : [15009, 0], "width" : 11,"height" : 4,"color" : "peachpuff"} ]'\
    $xcols_arg $ycols_arg --ylim 2.7 3.3 --xlim 14950 15050

plot "$PREFIX-vporymm-{0..2}.csv" "fig-vpor256" "Frequency (GHz)" "Time (us)" "256-bit VPOR Transitions (no vzeroupper)" \
    $xcols_arg $ycols_arg --ylim 0 4

plot "$PREFIX-vporzmm-{0..2}.csv" "fig-vpor512" "Frequency (GHz)" "Time (us)" "512-bit VPOR Transitions (no vzeroupper)" \
    $xcols_arg $ycols_arg --ylim 0 4

plot "$PREFIX-vporzmm_vz100-{0..2}.csv" "fig-vporvz512-ipc" "Frequency (GHz)" "Time (us)" "512-bit VPOR Frequency Transitions" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 0 4 --ylabel2 IPC

plot "$PREFIX-vporxmm_vz100-{0..2}.csv" "fig-ipc-zoomed-xmm" "Frequency (GHz)" "Time (us)" "128-bit VPOR Transition Closeup" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 1.2 \
    --marker=. --marker2=. --legend-loc='upper right'

plot "$PREFIX-vporymm_vz100-{0..2}.csv" "fig-ipc-zoomed-ymm" "Frequency (GHz)" "Time (us)" "256-bit VPOR Transition Closeup" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 1.2 \
    --marker=. --marker2=. --legend-loc='upper right' --patches \
    '[{ "xy": [15000, 0], "width" : 9, "height" : 4,"color" : "thistle"}]'

plot "$PREFIX-vporzmm_vz100-{0..2}.csv" "fig-ipc-zoomed-zmm" "Frequency (GHz)" "Time (us)" "512-bit VPOR Transition Closeup" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 1.2 \
    --marker=. --marker2=. --legend-loc='upper right' --patches \
    '[{ "xy": [15000, 0], "width" : 9, "height" : 4,"color" : "thistle"},
    { "xy"  : [15009, 0], "width" :11, "height" : 4,"color" : "peachpuff"},
    { "xy"  : [15020, 0], "width" : 9.5, "height" : 4,"color" : "darkseagreen"},
    { "xy"  : [15029.5, 0], "width" :70.5, "height" : 4,"color" : "darkturquoise"} ]'

# 8 us version of zmm lat
plot "$PREFIX-vporzmm_vz100-10us-{0..2}.csv" "fig-ipc-zoomed-zmm-8us" "Frequency (GHz)" "Time (us)" "512-bit VPOR Transition (8 us sampling)" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 1.2 \
    --marker=. --marker2=. --legend-loc='upper right'

plot "$PREFIX-vporymm_tput_vz100-{0..2}.csv" "fig-ipc-zoomed-ymm-tput" "Frequency (GHz)" "Time (us)" "256-bit VPOR Transition w/ IPC" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 2 \
    --marker=. --marker2=. --legend-loc='upper right'

plot "$PREFIX-vporzmm_tput_vz100-{0..2}.csv" "fig-ipc-zoomed-zmm-tput" "Frequency (GHz)" "Time (us)" "512-bit VPOR Transition w/ IPC" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 2 \
    --marker=. --marker2=. --legend-loc='upper right'

plot "$PREFIX-vpermdzmm_vz100-{0..2}.csv" "fig-vpermd-ipc-zoomed-tput" "Frequency (GHz)" "Time (us)" "512-bit VPERMD Transition IPC Closeup" \
    $xcols_arg $ycols_arg --cols2-by-name "IPC" --ylim 2.7 3.3 --xlim 14950 15150 --ylabel2 IPC --ylim2 0 2 \
    --marker=. --marker2=. --legend-loc='upper right' --patches \
    '[{ "xy": [15000, 0], "width" : 9, "height" : 4,"color" : "thistle"},
    { "xy"  : [15009, 0], "width" :11, "height" : 4,"color" : "peachpuff"},
    { "xy"  : [15020, 0], "width" : 9.5, "height" : 4,"color" : "darkseagreen"},
    { "xy"  : [15029.5, 0], "width" :70.5, "height" : 4,"color" : "darkturquoise"} ]'

#for p in $period_list; do
for p in 760; do
    plot "$PREFIX-vporzmm_vz100-period$p-{0..2}.csv" "fig-vporvz512-ipc-p$p" "Frequency (GHz)" "Time (us)" "Transition Closeup: $p μs Period" \
        $xcols_arg $ycols_arg --cols2-by-name "IPC" --xlim 7550 7700 --ylim 2.7 3.3 --ylabel2 IPC --ylim2 0 1.2 --marker=. --marker2=.
done
