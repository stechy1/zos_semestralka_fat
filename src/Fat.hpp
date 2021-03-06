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

#ifndef SEMESTRALKA_FAT_HPP
#define SEMESTRALKA_FAT_HPP

#include <vector>
#include <memory>
#include <cmath>
#include <mutex>

#define PATH_SEPARATOR "/"

//struktura na boot record
struct boot_record {
    char volume_descriptor[251];              //popis
    int fat_type;                             //typ FAT - pocet clusteru = 2^fat_type (priklad FAT 12 = 4096)
    int fat_copies;                           //kolikrat je za sebou v souboru ulozena FAT
    unsigned int cluster_size;                //velikost clusteru ve znacich (n x char) + '/0' - tedy 128 znamena 127 vyznamovych znaku + '/0'
    unsigned long root_directory_max_entries_count;    //pocet polozek v root_directory = pocet souboru MAXIMALNE, nikoliv aktualne - pro urceni kde zacinaji data - resp velikost root_directory v souboru
    unsigned int cluster_count;               //pocet pouzitelnych clusteru (2^fat_type - reserved_cluster_count)
    unsigned int reserved_cluster_count;      //pocet rezervovanych clusteru pro systemove polozky
    char signature[4];                        //pro vstupni data od vyucujicich konzistence FAT - "OK","NOK","FAI" - v poradku / FAT1 != FAT2 != FATx / FAIL - ve FAT1 == FAT2 == FAT3, ale obsahuje chyby, nutna kontrola
};

//struktura na root directory
struct root_directory {
    char file_name[13];             //8+3 format + '/0' = 12 -> 13
    char file_mod[10];              //unix atributy souboru+ '/0' rvxrvxrvx
    short file_type;                //1 = soubor, 2 = adresar, 0 = nepouzito
    long file_size;                 //pocet znaku v souboru
    unsigned int first_cluster;     //cluster ve FAT, kde soubor zacina - POZOR v cislovani root_directory ma prvni cluster index 0 (viz soubor a.txt)
};

class Fat {

public:

    static const int FAT_UNUSED = 65535;
    static const int FAT_FILE_END = 65534;
    static const int FAT_BAD_CLUSTER = 65533;
    static const int FAT_DIRECTORY_CONTENT = 65532;

    // Definice vlastnosti boot recordu
    static const unsigned int FAT_COPIES = 2;
    static const unsigned int FAT_TYPE = 12; // == FAT12, je možné i FAT32...
    static const unsigned int CLUSTER_SIZE = 150;
    static const unsigned int RESERVER_CLUSTER_COUNT = 10;
    static const long ROOT_DIRECTORY_MAX_ENTRIES_COUNT = 3;
    static const unsigned int FAT_SIZE = 1 << FAT_TYPE;
    static const unsigned int CLUSTER_COUNT = FAT_SIZE - RESERVER_CLUSTER_COUNT;
    // Oddělovač
    //static constexpr const std::string PATH_SEPARATOR = "/";
    static const unsigned short SPACE_SIZE = 4;

    // File typy
    static const short FILE_TYPE_FILE = 1;
    static const short FILE_TYPE_DIRECTORY = 2;

    // Index prvniho clusteru, který je použitelný pro zápis dat
    // nultý index obsahuje "root_directory"
    static const unsigned int FAT_FIRST_CONTENT_INDEX = 1;

    friend class Defragmenter;

    Fat(std::string &t_filePath);

    Fat(const Fat &other) = delete;

    Fat &operator=(const Fat &other) = delete;

    Fat(Fat &&other) = delete;

    Fat &operator=(Fat &&other) = delete;

    ~Fat();

    // Uživatelské funkce
    void loadFat();

    std::shared_ptr<root_directory> findFileDescriptor(const std::string &t_path);

    std::vector<unsigned int> getClusters(std::shared_ptr<root_directory> t_fileEntry);

    void tree();

    void createEmptyFat();

    void createDirectory(const std::string &t_path, const std::string &t_addr);

    void deleteDirectory(const std::string &t_pseudoPath);

    void insertFile(const std::string &t_filePath, const std::string &t_pseudoPath);

    void deleteFile(const std::string &t_pseudoPath);

    const std::string getClusterContent(const unsigned int t_index);

    const std::vector<std::string> readFileContent(const std::shared_ptr<root_directory> t_rootDirectory);

    const std::vector<std::string> readFileContent(const unsigned int t_index, const long t_fileSize);

    void writeClusterContent(const unsigned int t_index, const std::string &t_data);

    void save();

    // Metody pro výpis informací
    void printBootRecord();

    void printRootDirectories();

    void printRootDirectory(const std::shared_ptr<root_directory> t_rootDirectory);

    void printClustersContent();

    void printFileContent(const std::vector<std::string> &t_fileContent);

    void printSubTree(const std::shared_ptr<root_directory> t_rootDirectory, const unsigned int t_depth);

private:
    std::string m_FilePath = "";
    FILE *m_FatFile;
    std::unique_ptr<boot_record> m_BootRecord;
    std::shared_ptr<root_directory> m_RootFile;
    std::vector<std::shared_ptr<root_directory>> m_RootDirectories;
    unsigned int **m_fatTables;
    unsigned int *m_workingFatTable;

    long m_FatStartIndex = 0;
    long m_ClustersStartIndex = 0;

    mutable std::recursive_mutex m_recursive_mutex;

    void openFile();

    void closeFile();

    // Načítací metody
    void loadBootRecord();

    void loadFatTable();

    std::vector<std::shared_ptr<root_directory>> loadDirectory(unsigned int t_offset);

    // Ukládací metody
    void saveBootRecord();

    void saveFatTables();

    void saveFatPiece(long t_offset, unsigned int t_value);

    void saveRootDirectory();

    void saveClusterWithFiles(std::vector<std::shared_ptr<root_directory>> t_rootDirectory, unsigned int t_offset);

    // Privátní výkonné metody (dělají nějakou prácí)
    void setFatPiece(long t_offset, unsigned int t_value);

    void clearFatRecord(long t_offset);

    std::shared_ptr<root_directory>
    makeFile(const std::string &t_fileName, const std::string &t_fileMod, long t_fileSize, short t_fileType,
             unsigned int t_firstCluster);

    std::shared_ptr<root_directory> findFileDescriptor(const std::shared_ptr<root_directory> &t_parent,
                                                       const std::vector<std::shared_ptr<root_directory>> &t_rootDirectory,
                                                       const std::string &t_path);

    void writeFile(FILE *t_file, std::shared_ptr<root_directory> t_fileEntry);

    // Pomocné metody pro získání offsetů
    const unsigned int getFatStartIndex();

    const unsigned int getClustersStartIndex();

    const unsigned int getClusterStartIndex(unsigned int t_offset);

    const unsigned int getFreeCluster();
    const unsigned int getFreeCluster(const unsigned int t_offset);

    const std::vector<unsigned int> getFreeClusters(const unsigned int t_count);

};

#endif //SEMESTRALKA_FAT_HPP
