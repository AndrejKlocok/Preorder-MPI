#!/bin/bash
# Author:   Andrej Klocok, xkloco00
# Project:  Priradenie poradia preorder vrcholom
# File:     test.sh

#compile
mpic++ -std=c++11 --prefix /usr/local/share/OpenMPI -o pro pro.cpp

#number count
if [ $# -ne 1 ]; then
    echo "Usage: $0 [] - array, which represents tree"
    exit
fi;

array=$1
processors=$(( 2*${#array}-2 ))
len=$((${#array}))

if [ $len == 1 ]; then
    echo ${array[*]}
    exit
fi;

#run
mpirun --prefix /usr/local/share/OpenMPI -np $processors pro -arr $array

#clean
rm -f pro
