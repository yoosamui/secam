#!/bin/bash

path=$("pwd")
hostfile=$path$"/data/scripts/ipaddress"

ip=$(head -n 1 $hostfile);
out=$(nc -zv -w10 ${ip} 55499 2>&1)


if [[ $out == *"succeeded!"* || $out == *"open"* ]]; then
    echo 1
else
    echo 0
fi

