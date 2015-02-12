#!/bin/bash

die() {
    echo "$@" >&2
    exit 1
}

float_eval() {
    bash -c "enable -f $PWD/float_eval.so float_eval; float_eval '$@'; echo \$REPLY"
}

TESTS=(
    "2"             "2"
    "1+2"           "3"
    "3*2"           "6"
    "1+2*3"         "7"
    "1*2+3"         "5"
    "2*(2+3)"       "10"
    "(2+3)*3"       "15"
    "(1+(2*3)+1)*2" "16"
    "(1+2)*(2+3)"   "15"
    "1*(2+3)*6+7"   "37"
    "1+(2+3)*6+7"   "38"
    "(1+2)*(2+3)+1" "16"
    "-2"            "-2"
    "-1+2"          "1"
    "1-2"           "-1"
    "-1-2"          "-3"
    "-(1+2)"        "-3"
    "-1-(1+2)"      "-4"
    )

for (( i = 0; i < ${#TESTS[@]}; i+=2 )); do
    res=$(float_eval "${TESTS[i]}")
    [[ "${res%%.000000}" == "${TESTS[i+1]}" ]] ||
        die "FAIL : '${TESTS[i]}' give '$res' instead of '${TESTS[i+1]}'"
done

echo "Everything OK"
