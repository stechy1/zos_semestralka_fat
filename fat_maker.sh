#!/bin/bash

if [ "$#" -ne "1" ]; then
    echo "Zadejte nazev fatky"
    exit
fi

fatFile=$1
bin='zos_semestralka_fat'
sleepTime=1

# Vytvoření prázdné fatky
echo "Vytvarim novou fatku"
./${bin} ${fatFile} -n
sleep ${sleepTime}

# Vytvoření kořenové složky bin
echo "Vytvarim korenovou slozku bin"
./${bin} ${fatFile} -m / bin
sleep ${sleepTime}

# Vytvoření kořenové složky home
echo "Vytvarim korenovou slozku home"
./${bin} ${fatFile} -m / home
sleep ${sleepTime}

# Vložení souboru a.txt do kořenové složky
echo "Vkladam soubor a.txt do korenove slozky"
./${bin} ${fatFile} -a ./a.txt /a.txt
sleep ${sleepTime}

# Vytvoření uživatelské složky ve složce home
echo "Vytvarim uzivatelskou slozku 'petr'"
./${bin} ${fatFile} -m /home petr
sleep ${sleepTime}

# Vložení do uživatelské složky petra soubor b.txt
echo "Vkladam do uzivatelske slozky soubor b.txt"
./${bin} ${fatFile} -a ./b.txt /home/petr/b.txt
sleep ${sleepTime}

# Vložení do uživatelské složky petra soubor c.txt
echo "Vkladam do uzivatelske slozky soubor c.txt"
./${bin} ${fatFile} -a ./c.txt /home/petr/c.txt
sleep ${sleepTime}

# Vytvoření uživatelské složky ve složce home
echo "Vytvarim uzivatelskou slozku 'michal'"
./${bin} ${fatFile} -m /home michal
sleep ${sleepTime}

# Vytvoření uživatelské složky ve složce home
echo "Vytvarim uzivatelskou slozku 'filip'"
./${bin} ${fatFile} -m /home filip
sleep ${sleepTime}

# Vložení souboru a.txt do složky bin
echo "Vkladam do uzivatelske slozky soubor a.txt"
./${bin} ${fatFile} -a ./a.txt /bin/a.txt
sleep ${sleepTime}

# Vložení souboru b.txt do složky bin
echo "Vkladam do uzivatelske slozky soubor b.txt"
./${bin} ${fatFile} -a ./b.txt /bin/b.txt
sleep ${sleepTime}

# Vložení souboru c.txt do složky bin
echo "Vkladam do uzivatelske slozky soubor c.txt"
./${bin} ${fatFile} -a ./c.txt /bin/c.txt
sleep ${sleepTime}

# Smazání souboru b.txt ve složce bin
echo "Mazu soubor /bin/b.txt"
./${bin} ${fatFile} -f /bin/b.txt
sleep${sleepTime}

# Vložení velikého souboru f.txt na místo smazaného souboru b.txt pro vytvoření fragmentovaného souboru
echo "Vkladam velky soubor f.txt na misto smazaneho souboru /bin/b.txt"
./${bin} ${fatFile} -a ./f.txt /bin/f.txt

echo "Vytvoreni probehlo uspesne. Zde je stromova struktura:"
./${bin} ${fatFile} -p