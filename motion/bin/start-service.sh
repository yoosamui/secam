#!/bin/bash
## run while loop to display date and hostname on screen ##
while [ : ]
do
    clear
    tput cup 5 5
    date
    tput cup 6 5
    echo "Hostname : $(hostname)"
    sleep 1
done
exit

camera="-c=gate"
mode="-m=1"
ls
watch date
#./motion $camera $mode


