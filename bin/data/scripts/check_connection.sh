#!/bin/bash

ip=$(head -n 1 "/home/$USER/scripts/ipaddress");
out=$(nc -zv -w10 ${ip} 55499 2>&1)
#echo "---> $out"

if [[ $out == *"succeeded!"* || $out == *"open"* ]]; then
    echo 1
else
    echo 0
fi

