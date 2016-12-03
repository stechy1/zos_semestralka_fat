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


//void initEmptyRootDir() {
//    emptyRootDir = std::make_unique<root_directory>();
//
//    memset(emptyRootDir->file_name, '\0', sizeof(emptyRootDir->file_name));
//    strcpy(emptyRootDir->file_name, "empty");
//    emptyRootDir->file_size = 0;
//    emptyRootDir->file_type = 1;
//    memset(emptyRootDir->file_mod, '\0', sizeof(emptyRootDir->file_mod));
//    strcpy(emptyRootDir->file_mod, "rwxrwxrwx");
//    emptyRootDir->first_cluster = FAT_UNUSED;
//}

// Vyčistí záznamy celé fatky
//void clearFat(int offset) {
//    if (offset < 0 || offset > FAT_SIZE) {
//        return;
//    }
//
//    for (int i = offset; i<=FAT_SIZE; i++) {
//        Fat[i] = FAT_UNUSED;
//    }
//}

// Vytvoří nový boot record podle výchozích konstant
//void createNewBootRecord() {
//    br = std::make_unique<boot_record>();
//
//    memset(br->signature, '\0', sizeof(br->signature));
//    memset(br->volume_descriptor, '\0', sizeof(br->volume_descriptor));
//    strcpy(br->signature, "OK");
//    strcpy(br->volume_descriptor, "Volume descriptor");
//    br->fat_copies = FAT_COPIES;
//    br->fat_type = FAT_TYPE;
//    br->cluster_size = CLUSTER_SIZE;
//    br->cluster_count = CLUSTER_COUNT;
//    br->reserved_cluster_count = RESERVER_CLUSTER_COUNT;
//    br->root_directory_max_entries_count = ROOT_DIRECTORY_MAX_ENTRIES_COUNT;
//}

// Vytvoří novou fatku a zapíše jí do souboru
//void createEmptyFatTable() {
//    unlink(fileName.c_str());
//    FILE *fp = fopen(fileName.c_str(), "w+");
//
//    createNewBootRecord();
//    clearFat(0);
//
//    // Zápis boot recordu
//    fwrite(&(*br), sizeof(*br), 1, fp);
//
//    // Zápis čisté fatky
//    fwrite(&Fat, sizeof(Fat), 1, fp);
//    fwrite(&Fat, sizeof(Fat), 1, fp);


//    fclose(fp);
//}

//void writeFat() {
//    unlink(fileName.c_str());
//    FILE *fp = fopen(fileName.c_str(), "w+");
//
//    // Zápis boot recordu
//    fwrite(&(*br), sizeof(*br), 1, fp);
//
//    // Zápis N fatek
//    for (int i = 0; i < br->fat_copies; ++i) {
//        fwrite(&Fat, sizeof(Fat), 1, fp);
//    }
//
//
//
//    fclose(fp);
//}

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

    } else if (action == ACTION_M) { // Vytvoří nový adresář ADR v cestě ADR2

    } else if (action == ACTION_R) { // Smaže prázdný adresář ADR (ADR je plná cesta ve virtuální FAT)

    } else if (action == ACTION_L) { // Vypíše obsah souboru s1 na obrazovku (s1 je plná cesta ve virtuální FAT)

    } else if (action == ACTION_P) { // Vypíše obsah adresáře ve formátu +adresář, +podadresář cluster, ukončeno --, - soubor první_cluster počet_clusterů. Jeden záznam jeden řádek. Podadresáře odsazeny o /t:

    } else if (action == ACTION_N) { // Vytvoří novou čistou fatku

    } else if (action == ACTION_D) { // Vypíše obsah celé fatky
        Fat fat(fileName);
        fat.loadFat();
        fat.printBootRecord();
        fat.printRootDirectories();
    } else  {
        std::cout << "Neznámá akce" << std::endl;
        exit(1);
    }
}