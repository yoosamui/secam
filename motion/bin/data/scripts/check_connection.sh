#!/bin/bash

if [[ -z $1 ]] || [[ -z $2 ]]; then
    echo -1
    exit
fi 

out=$(nc -zv -w5 $1 $2 2>&1)
if [[ $out == *"succeeded!"* || $out == *"open"* ]]; then
    echo 1
else
    echo 0
fi

