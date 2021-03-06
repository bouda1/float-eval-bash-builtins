#!/bin/bash

: ${NB:=1000}
LC_ALL=C

die() {
    echo "$@" >&2
    exit 1
}

float_eval() {
    bash -c "enable -f $PWD/float_eval.so float_eval; float_eval -p 3 '$@'; echo \$REPLY"
}

float_eval_bash () {
  local scale=2 res=

  if [[ $# > 0 ]]; then
      res=$(bc -q <<< "scale=$scale ; $*" 2> /dev/null)
  fi

  [[ "${res:0:1}" == "." ]] && res="0$res"

  REPLY="$res"
  [[ $res == 0 ]] && return 1
  return 0
}

TESTS=(
    "2"             "2"
    "1+2"           "3"
    "1+2+3"         "6"
    "3*2"           "6"
    "2**3"          "8"
    "1+2*3"         "7"
    "2*2+3"         "7"
    "2*2*2"         "8"
    "1+2*3*4+5"     "30"
    "3*2+3"         "9"
    "2*(2+3)"       "10"
    "(2+3)*3"       "15"
    "1+(3*-2)"      "-5"
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
    "10&12"         "8"
    "1+10&12/2"     "2"
    "10|12"         "14"
    "1+10|12/2"     "15"
    "1|10&12"       "9"
    "10^12"         "6"
    "1|10^12&5"     "15"
    "5%3"           "2"
    "8%3+1"         "3"
    "8%3*2"         "4"
    "2*8%3"         "1"
    "2+3*2%7"       "8"
    "1+(3)*(2%7)"   "7"
    "3*2*8%10+1"    "9"
    "1*2+(1+2)*(2+2*3*2%7*2)"    "38"
    "2||1"          "1"
    "0||1"          "1"
    "6&&2"          "1"
    "0&&2"          "0"
    "1<1"           "0"
    "1<2"           "1"
    "1<=1"          "1"
    "1-1<1"         "1"
    "1<2"           "1"
    "1==2"          "0"
    "1!=2"          "1"
    "1==1"          "1"
    "1!=1"          "0"
    "1!=1||1"       "1"
    "8>>2"          "2"
    "8<<2"          "32"
    "!(1<2)"        "0"
    "!0"            "1"
    "~0"            "-1"
    "2+~0"          "1"
    "2E+3"          "2000"
    "4E-3"          "0.004"
    "2.03E4"        "20300"
    "1E1+2"         "12"
    "1+2E0+3"       "6"
    "2E1*(2+3)"     "100"
    "(2+3E1)*3"     "96"
    "-2E1"          "-20"
    "1E1&12"        "8"
    "5E1%3"         "2"
    "1/-2"          "-0.500"
    "(1<<3)+1"      "9"
    "1+(2**3)*6+7"  "56"
    )

buffer_size=42
nb_test=${#TESTS[@]}
for (( i = 0; i < buffer_size ; i++ )); do
    TESTS[nb_test]+="+1"
done
TESTS[nb_test+1]+="$buffer_size"
(( nb_test+=2 ))
for (( i = 0; i < buffer_size ; i++ )); do
    TESTS[nb_test]+="1"
done
TESTS[nb_test+1]+="${TESTS[nb_test]}"

for (( i = 0; i < ${#TESTS[@]}; i+=2 )); do
    res=$(float_eval "${TESTS[i]}")
    [[ "${TESTS[i+1]}" == "${res%.*}" || "${TESTS[i+1]}" == "${res}" ]] ||
        die "FAIL : '${TESTS[i]}' give '$res' instead of '${TESTS[i+1]}'"
done

echo "Everything OK"


echo -e "\n============= Benchmarks for $NB iterations  ===============\n"
echo "--> Builtins"
bash -c "enable -f $PWD/float_eval.so float_eval; time for (( i = 0; i < $NB; i++ )); do float_eval '1+2'; done"

echo "--> Builtins one shot"
bash -c "enable -f $PWD/float_eval.so float_eval; for (( i = 0; i < $NB; i++ )); do to_eval+=( '1+2' ) ; done ; time float_eval \${to_eval[@]}"

[[ "$NO_BENCH_BC" ]] && exit 0

echo -e "\n--> Bash (bc)"
time for (( i = 0; i < $NB; i++ )); do
    float_eval_bash "1+2"
done
