#ifndef SEMESTRALKA_FAT_HPP
#define SEMESTRALKA_FAT_HPP


#include <vector>
#include <memory>

const int FAT_UNUSED = 65535;
const int FAT_FILE_END = 65534;
const int FAT_BAD_CLUSTER = 65533;

// Definice vlastnosti boot recordu
const unsigned int FAT_COPIES = 2;
const unsigned int FAT_TYPE = 12; // == FAT12, je možné i FAT32...
const unsigned int CLUSTER_SIZE = 128;
const unsigned int RESERVER_CLUSTER_COUNT = 10;
const long ROOT_DIRECTORY_MAX_ENTRIES_COUNT = 3;
const unsigned int FAT_SIZE = 2^FAT_TYPE;
const unsigned int CLUSTER_COUNT = FAT_SIZE - RESERVER_CLUSTER_COUNT;
// Oddělovač
const std::string PATH_SEPARATOR = "/";

// File typy
const short FILE_TYPE_FILE = 0;
const short FILE_TYPE_DIRECTORY = 1;
const short FILE_TYPE_UNUSED = 2;

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
struct root_directory{
    char file_name[13];             //8+3 format + '/0' = 12 -> 13
    char file_mod[10];              //unix atributy souboru+ '/0' rvxrvxrvx
    short file_type;                //0 = soubor, 1 = adresar, 2 = nepouzito
    long file_size;                 //pocet znaku v souboru
    unsigned int first_cluster;     //cluster ve FAT, kde soubor zacina - POZOR v cislovani root_directory ma prvni cluster index 0 (viz soubor a.txt)
};

class Fat {

public:
    Fat(std::string &t_filePath);
    Fat(const Fat &other) = delete;
    Fat& operator=(const Fat& other) = delete;
    Fat(Fat &&other) = delete;
    Fat& operator=(Fat&& other) = delete;

    ~Fat() = default;

    void loadFat();

    // Print methods
    void printBootRecord();
    void printRootDirectories();
    void printRootDirectory(std::shared_ptr<root_directory> t_rootDirectory);
    void printClustersContent();
    void printFileContent(std::shared_ptr<root_directory> t_rootDirectory);

    std::shared_ptr<root_directory> findFirstCluster(const std::string &path);


private:
    std::string mFilePath = "";
    std::unique_ptr<FILE> m_fatFile;
    std::unique_ptr<boot_record> mBootRecord;
    std::vector<std::shared_ptr<root_directory>> mRoot_directories;
    std::unique_ptr<unsigned int[]> mFatTable;

    long m_FatStartIndex = 0;
    long m_RootDirectoryStartIndex = 0;
    long m_ClustersStartIndex = 0;

    void loadBootRecord();
    void loadFatTable();
    std::vector<std::shared_ptr<root_directory>> loadRootDirectories(long offset);

    std::shared_ptr<root_directory> findFirstCluster(
            const std::vector<std::shared_ptr<root_directory>> &t_Root_directories, const std::string &path);

    const long getFatStartIndex();
    const long getRootDirectoryStartIndex();
    const long getClustersStartIndex();
    const long getClusterStartIndex(int offset);

};

#endif //SEMESTRALKA_FAT_HPP
