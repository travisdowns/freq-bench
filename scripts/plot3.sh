# plot three different tests, given as $1, $2 and $3 against each other

source "$(dirname "$0")/common.sh"

if [ "$#" -ne 3 ]; then
    echo -e "Usage: plot3.sh TEST1 TEST2 TEST3\n\tWhere TEST1, TEST2 and TEST3 are test names"
    exit 1
fi

FILE1="$TDIR/$1"
FILE2="$TDIR/$2"
FILE3="$TDIR/$3"

echo "Writing temporary results to $FILE1, $FILE2 and $FILE3. Plot to ${OUT[@]}" >&2

./bench "$1" $START $STOP > "$FILE1"
./bench "$2" $START $STOP > "$FILE2"
./bench "$3" $START $STOP > "$FILE3"

"$PLOTPY" "$FILE1" "$FILE2" "$FILE3" "${COMMON_ARGS[@]}"

