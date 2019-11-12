# plot one tests, with performance counters, which you set in env var CPU_COUNTERS

source "$(dirname "$0")/common.sh"

if [ "$#" -ne 1 ]; then
    echo -e "Usage: plot1.sh TEST1\n\tWhere TEST1 is the test names"
    exit 1
fi

FILE1="$TDIR/$1"

echo "Writing temporary results to $FILE1. Plot to ${OUT[@]}" >&2

./bench "$1" $START $STOP > "$FILE1"

"$PLOTPY" "$FILE1" "${COMMON_ARGS[@]}"

