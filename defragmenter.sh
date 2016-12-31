#!/bin/bash

if [ "$#" -ne "1" ]; then
    echo "Zadejte nazev fatky"
    exit
fi

file_name=$1
echo ${file_name}

status="3"

./zos_semestralka_fat ${file_name} -p
while [ "$status" -eq "3" ]; do
    ./zos_semestralka_fat ${file_name} -b
    status=$?
done
./zos_semestralka_fat ${file_name} -p