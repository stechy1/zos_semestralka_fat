#include <iostream>
#include <cstring>
#include <unistd.h>
#include "Fat.hpp"

// Definice proveditelných akcí
const std::string ACTION_A = "-a";
const std::string ACTION_F = "-f";
const std::string ACTION_C = "-c";
const std::string ACTION_M = "-m";
const std::string ACTION_R = "-r";
const std::string ACTION_L = "-l";
const std::string ACTION_P = "-p";
// Moje definice akcí
const std::string ACTION_N = "-n";
const std::string ACTION_D = "-d";

const std::string PATH_NOT_FOUND = "PATH NOT FOUND";

void printClusters(std::vector<unsigned int> clusters) {
    for (auto &&cluster : clusters) {
        std::cout << cluster << ", ";
    }

}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        perror("Příliš málo argumentů na můj vkus. Zkuste to s větším počtem. Končím!!!");
        exit(1);
    }

    std::string fileName(argv[1]);
    std::string action(argv[2]);

    if (action == ACTION_A) {        // Nahraje soubor z adresáře do cesty virtuální FAT tabulky

    } else if (action == ACTION_F) { // Smaže soubor s1 z vaseFAT.dat (s1 je plná cesta ve virtuální FAT)

    } else if (action == ACTION_C) { // Vypíše čísla clusterů, oddělené dvojtečkou, obsahující data souboru s1 (s1 je plná cesta ve virtuální FAT)
        Fat fat(fileName);
        fat.loadFat();

        std::string filePath = argv[3];
        auto rootDirectory = fat.findFirstCluster(filePath);
        auto clusters = fat.getClusters(rootDirectory);

        std::cout << filePath << ": ";
        printClusters(clusters);

    } else if (action == ACTION_M) { // Vytvoří nový adresář ADR v cestě ADR2
        Fat fat(fileName);
        fat.loadFat();
        std::string addrPath = argv[3];
        std::string addr = argv[4];

        fat.createDirectory(addrPath, addr);
        fat.save();

    } else if (action == ACTION_R) { // Smaže prázdný adresář ADR (ADR je plná cesta ve virtuální FAT)

    } else if (action == ACTION_L) { // Vypíše obsah souboru s1 na obrazovku (s1 je plná cesta ve virtuální FAT)
        Fat fat(fileName);
        fat.loadFat();
        std::string filePath = argv[3];
        try {
            auto rootDirectory = fat.findFirstCluster(filePath);
            fat.printFileContent(rootDirectory);
        } catch (const std::runtime_error &ex) {
            std::cout << PATH_NOT_FOUND << std::endl;
        }
    } else if (action == ACTION_P) { // Vypíše obsah adresáře ve formátu +adresář, +podadresář cluster, ukončeno --, - soubor první_cluster počet_clusterů. Jeden záznam jeden řádek. Podadresáře odsazeny o /t:

    } else if (action == ACTION_N) { // Vytvoří novou čistou fatku
        Fat fat(fileName);
        fat.createEmptyFat();
        fat.save();
    } else if (action == ACTION_D) { // Vypíše obsah celé fatky
        Fat fat(fileName);
        fat.loadFat();

        fat.printBootRecord();
        fat.printRootDirectories();
        fat.printClustersContent();
    } else  {
        std::cout << "Konec" << std::endl;
        exit(0);
    }
}