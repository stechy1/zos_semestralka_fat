#!/bin/bash

if [ "$#" -ne "1" ]; then
    echo "Zadejte nazev fatky"
    exit
fi

file_name=$1
echo ${file_name}

status="5"

while [ "$status" -eq "5" ]; do
    ./zos_semestralka_fat ${file_name} -b
    status=$?
    sleep 0.1
done