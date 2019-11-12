# plot three different tests, given as $1, $2 and $3 against each other

source "$(dirname "$0")/common.sh"

if [ "$#" -ne 4 ]; then
    echo -e "Usage: plot4.sh TEST1 ... TEST4\n\tWhere TEST{1,2,3,4} are test names"
    exit 1
fi

FILE1="$TDIR/$1"
FILE2="$TDIR/$2"
FILE3="$TDIR/$3"
FILE4="$TDIR/$4"

echo "Writing temporary results to $FILE1, $FILE2 and $FILE3. Plot to ${OUT[@]}" >&2

./bench "$1" $START $STOP > "$FILE1"
./bench "$2" $START $STOP > "$FILE2"
./bench "$3" $START $STOP > "$FILE3"
./bench "$4" $START $STOP > "$FILE4"

"$PLOTPY" "$FILE1" "$FILE2" "$FILE3" "$FILE4" "${COMMON_ARGS[@]}"

