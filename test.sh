#!/bin/bash

test_expression() {
  exp=$1
  val=$2

  echo "$exp" | ./cc > out.s
  if [ $? -ne 0 ]; then
    echo failed to generate assembly from "$exp"
    rm out.s
    exit 1
  fi

  gcc out.s -o out
  if [ $? -ne 0 ]; then
    echo failed to compile "$exp"
    cat out.s
    rm out.s
    exit 1
  fi

  ./out
  ret=$?

  if [ $ret -ne $val ]; then
    echo "$exp" should be $val, but got $ret.
    cat out.s
    rm out.s out
    exit 1
  fi

  rm out.s out
}

test_error() {
  exp=$1
  msg=$2

  echo "$exp" | ./cc > /dev/null 2> stderr.txt
  if [ $? -ne 1 ]; then
    echo "$exp" should cause exit status 1, but got 0.
    rm stderr.txt
    exit 1
  fi

  echo "$msg" | diff - stderr.txt > /dev/null
  ret=$?

  if [ $ret -ne 0 ]; then
    echo "$exp" should raise \""$msg"\", but got \""$(cat stderr.txt)"\".
    rm stderr.txt
    exit 1
  fi

  rm stderr.txt
}

test_expression "2" 2
test_expression "71" 71
test_expression "234" 234

test_expression "2 + 3" 5
test_expression "123 + 43 + 1 + 21" 188

test_expression "11 - 7" 4
test_expression "123 - 20 - 3" 100
test_expression "123 + 20 - 3 + 50 - 81" 109

test_expression "3 * 5" 15
test_expression "3 * 5 + 7 * 8 - 3 * 4" 59

test_expression "(3 - 5) * 3 + 7" 1
test_expression "(((123)))" 123

test_expression "6 / 2" 3
test_expression "6 / (2 - 3) + 7" 1
test_expression "123 % 31" 30
test_expression "32 / 4 + 5 * (8 - 5) + 5 % 2" 24

test_expression "3 * 5 == 7 + 8" 1
test_expression "3 * 5 == 123" 0
test_expression "3 * 5 != 7 + 8" 0
test_expression "3 * 5 != 123" 1

test_expression "5 * 7 < 36" 1
test_expression "5 * 7 < 35" 0
test_expression "5 * 7 > 35" 0
test_expression "5 * 7 > 34" 1

test_expression "5 * 7 <= 35" 1
test_expression "5 * 7 <= 34" 0
test_expression "5 * 7 >= 36" 0
test_expression "5 * 7 >= 35" 1

test_expression "1 && 1" 1
test_expression "0 && 1" 0
test_expression "1 && 0" 0
test_expression "0 && 0" 0
test_expression "3 * 7 > 20 && 5 < 10 && 6 + 2 <= 3 * 3 && 5 * 2 % 3 == 1" 1
test_expression "3 * 7 > 20 && 5 < 10 && 6 + 2 >= 3 * 3 && 5 * 2 % 3 == 1" 0

test_expression "1 || 1" 1
test_expression "0 || 1" 1
test_expression "1 || 0" 1
test_expression "0 || 0" 0
test_expression "8 * 7 < 50 || 10 > 2 * 5 || 3 % 2 == 1 || 5 * 7 < 32" 1
test_expression "8 * 7 < 50 || 10 > 2 * 5 || 3 % 2 != 1 || 5 * 7 < 32" 0
test_expression "3 * 7 > 20 && 2 * 4 <= 7 || 8 % 2 == 0 && 5 / 2 >= 2" 1

test_expression "5*4" 20
test_expression "5  *  4" 20
test_expression "  5 * 4  " 20

test_expression "+5" 5
test_expression "5 + (-5)" 0
test_expression "3 - + - + - + - 2" 5

test_expression "!0" 1
test_expression "!1" 0
test_expression "!!0" 0
test_expression "!(2 * 3 <= 2 + 3)" 1

test_expression "3 > 2 ? 13 : 31" 13
test_expression "3 < 2 ? 13 : 31" 31
test_expression "1 ? 1 ? 6 : 7 : 1 ? 8 : 9" 6
test_expression "1 ? 0 ? 6 : 7 : 1 ? 8 : 9" 7
test_expression "0 ? 1 ? 6 : 7 : 1 ? 8 : 9" 8
test_expression "0 ? 1 ? 6 : 7 : 0 ? 8 : 9" 9

test_expression "1 << 6" 64
test_expression "64 >> 6" 1
test_expression "64 >> 8" 0
test_expression "41 << 2" 164
test_expression "41 >> 3" 5

test_expression "183 & 109" 37
test_expression "183 | 109" 255
test_expression "183 ^ 109" 218

test_expression "~183 & 255" 72

test_error "abc" "error: unexpected character."
test_error "2 * (3 + 4" "error: tRPAREN is expected."
test_error "5 + *" "error: unexpected primary expression."
test_error "5 (4)" "error: invalid expression."
test_error "1 ? 2" "error: tCOLON is expected."