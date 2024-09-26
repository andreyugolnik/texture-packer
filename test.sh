#!/bin/bash

if [[ $# -eq 0 ]]; then
    echo "Usage:"
    echo "   $0 <directory/to/test/files>"
    exit 1
fi

test_dir=$1

separator() {
    echo ""
    echo "---------------------------------------------------------------------"
    echo ""
}

testError() {
    error_code=$?
    if [ $error_code -ne 0 ]; then
        echo ""
        echo "---------------------------------------------------------------------"
        echo "--- Test failed ($error_code). Stopped!"
        echo "---------------------------------------------------------------------"
        exit $error_code
    fi
}

doPacking() {
    echo "---------------------------------------------------------------------"
    echo "--- Start packing"
    echo "---------------------------------------------------------------------"
    echo ""
    while IFS= read -r -d $'\0' i; do
        echo "---------------------------------------------------------------------"
        echo "--- Packing: '$i'"
        echo "---------------------------------------------------------------------"

        if [ -f ./texpacker-old ]; then
            ./texpacker-old $@ "$i" -o "$i-old.png"
            testError
            separator
        fi

        if [ -f ./texpacker ]; then
            ./texpacker $@ "$i" -o "$i.png"
            testError
            separator
        fi

        echo ""
        echo ""
        echo ""
    done < <(find $test_dir -type d -maxdepth 1 -not -path $test_dir -print0)
}

doTests() {
    NC='\033[0m'
    RED='\033[0;31m'
    GREEN='\033[0;32m'

    local px_diff="0"
    local files_total=0
    local files_better=0
    local files_worse=0

    echo "---------------------------------------------------------------------"
    echo "--- Compare results"
    echo "---------------------------------------------------------------------"
    echo ""
    while IFS= read -r -d $'\0' i; do
        echo "---------------------------------------------------------------------"

        local file_old="$i-old.png"
        local file_new="$i.png"

        if [ ! -f "$file_old" -o ! -f "$file_new" ]; then
            exit 2
        fi

        dim_old=$(magick identify -format "%w * %h" "$file_old")
        echo " [$dim_old]: '$file_old'"

        dim_new=$(magick identify -format "%w * %h" "$file_new")
        echo " [$dim_new]: '$file_new'"

        diff=$(echo "$dim_new - $dim_old" | bc)
        px_diff="${px_diff} + $diff"

        # echo "100 * ($dim_new) / ($dim_old) - 100"
        local percent=$(echo "100 * ($dim_new) / ($dim_old) - 100" | bc -l)

        files_total=$((files_total + 1))

        if [ $diff == 0 ]; then
            echo "Unchanged"
        elif [ $diff -gt 0 ]; then
            files_worse=$((files_worse + 1))
            printf "Worse: ${RED}+%'d${NC} (${RED}+%.1f%%${NC})\n" $diff $percent
        else
            files_better=$((files_better + 1))
            printf "Better: ${GREEN}%'d${NC} (${GREEN}%.1f%%${NC})\n" $diff $percent
        fi

        echo ""
    done < <(find $test_dir -type d -maxdepth 1 -not -path $test_dir -print0)

    echo -e "Out of a total of $files_total files, ${GREEN}$files_better${NC} packed better + $(($files_total - $files_worse - $files_better)) unchanged, ${RED}$files_worse${NC} packed worse."

    # echo "'$px_diff'"
    px_diff=$(echo "$px_diff" | bc)
    str="The total pixel difference across all files is"
    if [ $px_diff == 0 ]; then
        echo "$str no difference"
    elif [ $px_diff -gt 0 ]; then
        printf "$str ${RED}+%'d${NC}.\n" $px_diff
    else
        printf "$str ${GREEN}%'d${NC}.\n" $px_diff
    fi
}

doPacking ${@:2}
if [ $? -ne 0 ]; then
    exit $?
fi

if [ -f ./texpacker-old -a -f ./texpacker ]; then
    doTests
fi
