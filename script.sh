#!/bin/bash

C=$1
B=$2
H=$3
R=$4

TARGET=tests/b${B}h${H}_r${R}.log

mkdir -p tests

for i in {0..99}; do
	echo './randomizer | ./splash' $B $R 25 $H
done | parallel -j${C} 2>> $TARGET
