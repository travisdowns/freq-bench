# plot two different tests, given as $1 and $2 against each other

source "$(dirname "$0")/common.sh"

if [ "$#" -ne 2 ]; then
    echo -e "Usage: plot2.sh TEST1 TEST2\n\tWhere TEST1 and TEST2 are test names"
    exit 1
fi

FILE1="$TDIR/$1"
FILE2="$TDIR/$2"

echo "Writing temporary results to $FILE1 and $FILE2. Plot to ${OUT[@]}" >&2

./bench "$1" $START $STOP > "$FILE1"
./bench "$2" $START $STOP > "$FILE2"

"$PLOTPY" "$FILE1" "$FILE2" "${COMMON_ARGS[@]}"

