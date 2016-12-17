#!/bin/bash

prg='zos_semestralka_fat'
fatFile='test.fat'

# Vytvoření prázdné fatky
echo "Vytvarim novou fatku"
./${prg} ${fatFile} -n
sleep 1

# Vytvoření kořenové složky bin
echo "Vytvarim korenovou slozku bin"
./${prg} ${fatFile} -m / bin
sleep 1

# Vytvoření kořenové složky home
echo "Vytvarim korenovou slozku home"
./${prg} ${fatFile} -m / home
sleep 1

# Vložení souboru a.txt do kořenové složky
echo "Vkladam soubor a.txt do korenove slozky"
./${prg} ${fatFile} -a ./a.txt /a.txt
sleep 1

# Vytvoření uživatelské složky ve složce home
echo "Vytvarim uzivatelskou slozku 'petr'"
./${prg} ${fatFile} -m /home petr
sleep 1

# Vložení do uživatelské složky petra soubor b.txt
echo "Vkladam do uzivatelske slozky soubor b.txt"
./${prg} ${fatFile} -a ./b.txt /home/petr/b.txt
sleep 1

# Vložení do uživatelské složky petra soubor c.txt
echo "Vkladam do uzivatelske slozky soubor c.txt"
./${prg} ${fatFile} -a ./c.txt /home/petr/c.txt
sleep 1

# Vytvoření uživatelské složky ve složce home
echo "Vytvarim uzivatelskou slozku 'michal'"
./${prg} ${fatFile} -m /home michal
sleep 1

# Vytvoření uživatelské složky ve složce home
echo "Vytvarim uzivatelskou slozku 'filip'"
./${prg} ${fatFile} -m /home filip
sleep 1

# Vložení souboru a.txt do složky bin
echo "Vkladam do uzivatelske slozky soubor a.txt"
./${prg} ${fatFile} -a ./a.txt /bin/a.txt
sleep 1

# Vložení souboru b.txt do složky bin
echo "Vkladam do uzivatelske slozky soubor b.txt"
./${prg} ${fatFile} -a ./b.txt /bin/b.txt
sleep 1

# Vložení souboru c.txt do složky bin
echo "Vkladam do uzivatelske slozky soubor c.txt"
./${prg} ${fatFile} -a ./c.txt /bin/c.txt
sleep 1

echo "Vytvoreni probehlo uspesne. Zde je stromova struktura:"
./${prg} ${fatFile} -p