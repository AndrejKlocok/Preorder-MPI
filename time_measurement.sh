#!/bin/bash

# Copyright (C) 2018   Tomáš Vlk
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# -----------------------------------------------------------------------------
# Preorder order of binary tree nodes in parallel
#
# Author:       Tomáš Vlk (xvlkto00)
# Date:         2018-04-18
# File:         test_measurement.sh
# Description:  Time measurement testing script
# -----------------------------------------------------------------------------


if [ $# -eq 2 ];then

    if [ -n "$1" ]; then
        test_count=$1;
    else
        test_count=100;
    fi;

    if [ -n "$2" ]; then
        nodes_count=$2;
    else
        nodes_count=20;
    fi;

else
    test_count=100;
    nodes_count=20;
fi;

# compile project
mpic++ --prefix /usr/local/share/OpenMPI -o pro pro.cpp -std=c++11 > /dev/null 2>&1

for (( j=18 ; $j<=${nodes_count} ; j=$j+1)); do

    # get random tree
    tree=$(cat /dev/urandom | tr -dc 'A-Z0-9' | fold -w ${j} | head -n 1)

    processor_count=$((2*$(echo -n ${tree} | wc -m)-2));

    echo "Tree: ${tree} nodes count ${j}";

    for (( a=0 ; $a<${test_count} ; a=$a+1 )); do

        if [ 0 -eq $(($a % 25)) ]; then
            echo "Test: ${a}"
        fi

        # run project
        echo ${tree} | mpirun --prefix /usr/local/share/OpenMPI -np ${processor_count} pro -arr $tree -time | grep 'Time'| cut -d ':' -f 2 >> ${j}_${test_count}.txt
    done
done

# cleanup
rm -f pro
