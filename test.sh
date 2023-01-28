#!/bin/bash

assert() {
	expected="$1"
	input="$2"

    echo "$input"
	./9cc "$input" > tmp.s
	cc -o tmp tmp.s 
	./tmp
	actual="$?"

	if [ "$actual" == "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$iput => $expected expected, but got $actual"
		exit 1
	fi
}

assert 0 0 
assert 42 42
assert 42 "12 + 35 - 5"
assert 128 "16 * 8"
assert 32 "8 + 12 * 2"
assert 10 "80 / 8"
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 12 '-20 + 32'
assert 0 '0==1'
assert 1 '42==42'
assert 1 '0!=1'
assert 0 '42!=42'
assert 1 '0<1'
assert 0 '1<1'
assert 0 '2<1'
assert 1 '0<=1'
assert 1 '1<=1'
assert 0 '2<=1'
assert 1 '1>0'
assert 0 '1>1'
assert 0 '1>2'
assert 1 '1>=0'
assert 1 '1>=1'
assert 0 '1>=2'

echo OK
