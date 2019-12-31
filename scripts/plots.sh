#!/bin/bash
set -e

# https://stackoverflow.com/a/12694189
SCRIPTDIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

PLOTPY="$SCRIPTDIR/plot-csv.py"
RESULTDIR="$SCRIPTDIR/../results"

# $1 input file(s)
# $2 output file (empty for default)
# $3 ylabel
# $4 xlabel
# $5 title
function plot {
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
    "$PLOTPY" "${input[@]}" "${OUT[@]}" --tight --ylabel "$3" --xlabel "$4" --title "$5" "${@:6}"
}

# scatter_args="--scatter --markersize=1"



plot "skx-vporymm_vz-{0..2}.csv" "fig-vporvz256" "Frequency (GHz)" "Time (us)" "256-bit vpor Frequency Transitions" \
   --xcols 1 --cols-by-name 'Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2' --ylim 0 4 $scatter_args


plot "skx-vporzmm_vz-{0..2}.csv" "fig-vporvz512" "Frequency (GHz)" "Time (us)" "512-bit vpor Frequency Transitions" \
   --xcols-by-name "us,us_1,us_2" --cols-by-name 'Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2' --ylim 0 4 $scatter_args

exit

plot "skx-vporzmm_vz-{0..2}.csv" "fig-vpor-zoomed" "Frequency (GHz)" "Time (us)" "Transition Detail" --marker=. \
   --patches \
'[{ "xy" : [15000, 0],  "width" : 9,"height" : 4,"color" : "thistle"},'\
'{ "xy" : [15009, 0], "width" : 11,"height" : 4,"color" : "peachpuff"} ]'\
    --xcols 1 --cols-by-name 'Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2' --ylim 2.7 3.3 --xlim 14950 15050

plot "skx-vporymm-{0..2}.csv" "fig-vpor256" "Frequency (GHz)" "Time (us)" "256-bit vpor Transitions (no vzeroupper)" \
   --xcols 1 --cols-by-name 'Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2' --ylim 0 4

plot "skx-vporzmm-{0..2}.csv" "fig-vpor512" "Frequency (GHz)" "Time (us)" "512-bit vpor Transitions (no vzeroupper)" \
   --xcols 1 --cols-by-name 'Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2' --ylim 0 4

plot "skx-vporzmm_vz100-{0..2}.csv" "fig-vporvz512" "Frequency (GHz)" "Time (us)" "512-bit vpor Frequency Transitions" \
   --xcols 1 --cols2-by-name "IPC" --cols-by-name 'Unhalt_GHz,Unhalt_GHz_1,Unhalt_GHz_2' --ylim 0 4 --verbose
