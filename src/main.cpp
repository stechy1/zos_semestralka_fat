/*
 *         Copyright 2016 Petr Štechmüller
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *         limitations under the License.
 */

#include <iostream>
#include <cstring>
#include "Fat.hpp"
#include "Defragmenter.hpp"
#include "ThreadPool.hpp"

// Definice proveditelných akcí
const std::string ACTION_A = "-a"; // Nahraje soubor z adresáře do cesty virtuální FAT tabulky
const std::string ACTION_F = "-f"; // Smaže soubor s1 z vaseFAT.dat (s1 je plná cesta ve virtuální FAT)
const std::string ACTION_C = "-c"; // Vypíše čísla clusterů, oddělené dvojtečkou, obsahující data souboru s1 (s1 je plná cesta ve virtuální FAT)
const std::string ACTION_M = "-m"; // Vytvoří nový adresář ADR v cestě ADR2
const std::string ACTION_R = "-r"; // Smaže prázdný adresář ADR (ADR je plná cesta ve virtuální FAT)
const std::string ACTION_L = "-l"; // Vypíše obsah souboru s1 na obrazovku (s1 je plná cesta ve virtuální FAT)
const std::string ACTION_P = "-p"; // Vypíše obsah adresáře ve formátu +adresář, +podadresář cluster, ukončeno --, - soubor první_cluster počet_clusterů. Jeden záznam jeden řádek. Podadresáře odsazeny o /t:
const std::string ACTION_B = "-b"; // Defragmentuje fatku
// Moje definice akcí
const std::string ACTION_N = "-n"; // Vytvoří novou čistou fatku
const std::string ACTION_D = "-d"; // Vypíše obsah celé fatky

void printClusters(std::vector<unsigned int> &clusters);

void a(Fat &fat, std::string &&filePath, std::string &&pseudoPath) {
    try {
        fat.insertFile(filePath, pseudoPath);
    } catch (std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}

void b(Fat &fat) {
    Defragmenter defragmenter(fat);
    std::cout << "Before" << std::endl;
    fat.tree();
    defragmenter.runDefragmentation();
    std::cout << "After" << std::endl;
    defragmenter.printTree();
}

void f(Fat &fat, std::string &&pseudoPath) {
    try {
        fat.deleteFile(pseudoPath);
    } catch (std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}

void c(Fat &fat, std::string &&pseudoPath) {
    try {
        auto rootDirectory = fat.findFileDescriptor(pseudoPath);

        auto clusters = fat.getClusters(rootDirectory);

        std::cout << pseudoPath << ": ";
        printClusters(clusters);
    } catch (std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}

void m(Fat &fat, std::string &&addrPath, std::string &&addr) {
    try {
        fat.createDirectory(addrPath, addr);
    } catch (std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}

void r(Fat &fat, std::string &&pseudoPath) {
    try {
        fat.deleteDirectory(pseudoPath);
    } catch (std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}

void l(Fat &fat, std::string &&pseudoPath) {
    try {
        auto rootDirectory = fat.findFileDescriptor(pseudoPath);
        auto content = fat.readFileContent(rootDirectory);
        fat.printFileContent(content);
    } catch (const std::runtime_error &ex) {
        std::cout << ex.what() << std::endl;
    }
}

void printClusters(std::vector<unsigned int> &clusters) {
    for (auto &&cluster : clusters) {
        std::cout << cluster << ", ";
    }

}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        perror("Příliš málo argumentů na můj vkus. Zkuste to s větším počtem. Končím!!!");
        exit(1);
    }

    std::string fileName(argv[1]);
    std::string action(argv[2]);

    Fat fat(fileName);

    if (action == ACTION_A) {        // Nahraje soubor z adresáře do cesty virtuální FAT tabulky
        a(fat, std::move(argv[3]), std::move(argv[4]));
    } else if (action == ACTION_F) { // Smaže soubor s1 z vaseFAT.dat (s1 je plná cesta ve virtuální FAT)
        f(fat, std::move(argv[3]));
    } else if (action ==
               ACTION_C) { // Vypíše čísla clusterů, oddělené dvojtečkou, obsahující data souboru s1 (s1 je plná cesta ve virtuální FAT)
        c(fat, std::move(argv[3]));
    } else if (action == ACTION_M) { // Vytvoří nový adresář ADR v cestě ADR2
        m(fat, std::move(argv[3]), std::move(argv[4]));
    } else if (action == ACTION_R) { // Smaže prázdný adresář ADR (ADR je plná cesta ve virtuální FAT)
        r(fat, std::move(argv[3]));
    } else if (action == ACTION_L) { // Vypíše obsah souboru s1 na obrazovku (s1 je plná cesta ve virtuální FAT)
        l(fat, std::move(argv[3]));
    } else if (action ==
               ACTION_P) { // Vypíše obsah adresáře ve formátu +adresář, +podadresář cluster, ukončeno --, - soubor první_cluster počet_clusterů. Jeden záznam jeden řádek. Podadresáře odsazeny o /t:
        fat.tree();
    } else if (action == ACTION_B) {
        b(fat);
    } else if (action == ACTION_N) { // Vytvoří novou čistou fatku
        fat.createEmptyFat();
        fat.save();
    } else if (action == ACTION_D) { // Vypíše obsah celé fatky
        fat.printBootRecord();
        fat.printRootDirectories();
        fat.printClustersContent();
    } else {
        std::cout << "Konec" << std::endl;
        exit(0);
    }
}