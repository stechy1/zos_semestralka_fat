#include <cstring>
#include <exception>
#include <assert.h>
#include "Fat.hpp"


Fat::Fat(std::string &t_filePath) : m_FilePath(t_filePath) {
    m_FatFile = fopen(m_FilePath.c_str(), "r+");
    if (m_FatFile == 0) {
        m_FatFile = fopen(m_FilePath.c_str(), "w+");
    }
}

Fat::~Fat() {

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        delete[] m_fatTables[i];
    }

    delete[] m_fatTables;

    fclose(m_FatFile);
}


// Public methods

// Načte fatku ze souboru
void Fat::loadFat() {
    loadBootRecord();
    loadFatTable();
    m_RootDirectories = loadRootDirectory(getRootDirectoryStartIndex());
}

std::shared_ptr<root_directory> Fat::findFirstCluster(const std::string &t_path) {
    return findFirstCluster(m_RootDirectories, t_path);
}

std::vector<unsigned int> Fat::getClusters(std::shared_ptr<root_directory> t_fileEntry) {
    assert(m_fatTables[0] != nullptr);

    auto clusters = std::vector<unsigned int>();
    auto workingCluster = t_fileEntry->first_cluster;
    bool isEOF;

    do {
        clusters.push_back(workingCluster);
        workingCluster = m_fatTables[0][workingCluster];
        isEOF = workingCluster == FAT_FILE_END;
    } while (!isEOF);

    return clusters;
}

void Fat::createDirectory(const std::string &t_path, const std::string &t_addr) {
    auto parentDirectory = findFirstCluster(t_path);

    std::shared_ptr<root_directory> root_dir = std::make_shared<root_directory>();
    memset(root_dir->file_name, '\0', sizeof(root_dir->file_name));
    memset(root_dir->file_mod, '\0', sizeof(root_dir->file_mod));
    strcpy(root_dir->file_name, t_addr.c_str());
    strcpy(root_dir->file_mod, "rwxrwxrwx");
    root_dir->file_size = m_BootRecord->cluster_size;
    root_dir->file_type = FILE_TYPE_DIRECTORY;
    root_dir->first_cluster = getFreeCluster();

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        m_fatTables[i][root_dir->first_cluster] = FILE_TYPE_DIRECTORY;
    }
}

void Fat::createEmptyFat() {
    m_BootRecord = std::make_unique<boot_record>();
    memset(m_BootRecord->signature, '\0', sizeof(m_BootRecord->signature));
    memset(m_BootRecord->volume_descriptor, '\0', sizeof(m_BootRecord->volume_descriptor));
    strcpy(m_BootRecord->signature, "OK");
    strcpy(m_BootRecord->volume_descriptor, "Prazdna fatka");
    m_BootRecord->fat_copies = FAT_COPIES;
    m_BootRecord->fat_type = FAT_TYPE;
    m_BootRecord->cluster_size = CLUSTER_SIZE;
    m_BootRecord->cluster_count = CLUSTER_COUNT;
    m_BootRecord->reserved_cluster_count = RESERVER_CLUSTER_COUNT;
    m_BootRecord->root_directory_max_entries_count = ROOT_DIRECTORY_MAX_ENTRIES_COUNT;

    m_fatTables = new unsigned int *[m_BootRecord->fat_copies];
    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        m_fatTables[i] = new unsigned int[m_BootRecord->cluster_count];
        for (int j = 0; j < m_BootRecord->cluster_count; ++j) {
            m_fatTables[i][j] = FAT_UNUSED;
        }
    }

    std::fseek(m_FatFile, getClusterStartIndex(m_BootRecord->cluster_count) + m_BootRecord->cluster_size - 1, SEEK_SET);
    std::fputc('\0', m_FatFile);
}

void Fat::save() {
    saveBootRecord();
    saveFatTables();
    saveRootDirectory();
}


// Print methods

// Vypíše obsah boot recordu
void Fat::printBootRecord() {
    assert(m_BootRecord != nullptr);

    printf("-------------------------------------------------------- \n");
    printf("BOOT RECORD \n");
    printf("-------------------------------------------------------- \n");
    printf("volume_descriptor :%s\n",m_BootRecord->volume_descriptor);
    printf("fat_type :%d\n",m_BootRecord->fat_type);
    printf("fat_copies :%d\n",m_BootRecord->fat_copies);
    printf("cluster_size :%d\n",m_BootRecord->cluster_size);
    printf("root_directory_max_entries_count :%ld\n",m_BootRecord->root_directory_max_entries_count);
    printf("cluster count :%d\n",m_BootRecord->cluster_count);
    printf("reserved clusters :%d\n",m_BootRecord->reserved_cluster_count);
    printf("signature :%s\n",m_BootRecord->signature);
}

// Vypíše všechny root directories
void Fat::printRootDirectories() {
    printf("-------------------------------------------------------- \n");
    printf("ROOT DIRECTORY \n");
    printf("-------------------------------------------------------- \n");

    for (auto &&rootDirectory : m_RootDirectories) {
        printRootDirectory(rootDirectory);
    }
}

// Vypíše obsah požadované boot directory
void Fat::printRootDirectory(std::shared_ptr<root_directory> t_rootDirectory) {
    printf("file_name :%s\n",t_rootDirectory->file_name);
    printf("file_mod :%s\n",t_rootDirectory->file_mod);
    printf("file_type :%d\n",t_rootDirectory->file_type);
    printf("file_size :%li\n",t_rootDirectory->file_size);
    printf("first_cluster :%d\n",t_rootDirectory->first_cluster);
}

// Vypíše obsahy clusterů, které obsahují soubory
void Fat::printClustersContent() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    auto *p_cluster = new char[m_BootRecord->cluster_size];
    std::memset(p_cluster, '\0', sizeof(p_cluster));
    std::fseek(m_FatFile, static_cast<int>(getClustersStartIndex()), SEEK_SET);

    for (int i = 0; i < m_BootRecord->cluster_count; ++i) {
        std::fread(p_cluster, sizeof(char) * m_BootRecord->cluster_size, 1, m_FatFile);

        if (p_cluster[0] == '\0') {
            printf("Cluster %d: skip\n", i);
            continue;
        }

        printf("Cluster %d: %s\n", i, p_cluster);
    }
}

// Vypíše obsah celého souboru
void Fat::printFileContent(std::shared_ptr<root_directory> t_rootDirectory) {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    auto *p_cluster = new char[m_BootRecord->cluster_size];
    std::memset(p_cluster, '\0', sizeof(p_cluster));
    auto workingCluster = t_rootDirectory->first_cluster;
    bool isEOF;

    do {
        std::fseek(&(*m_FatFile), getClusterStartIndex(workingCluster), SEEK_SET);
        std::fread(p_cluster, sizeof(char) * m_BootRecord->cluster_size, 1, m_FatFile);
        
        printf("Cluster %d: %s\n", workingCluster, p_cluster);
        workingCluster = m_fatTables[0][workingCluster];
        isEOF = workingCluster == FAT_FILE_END;

    } while (!isEOF);

    delete[] p_cluster;
}

// Private methods

// Načte boot record
void Fat::loadBootRecord() {
    std::fseek(m_FatFile, 0, SEEK_SET);
    m_BootRecord = std::make_unique<boot_record>();

    std::fread(&(*m_BootRecord), sizeof(struct boot_record), 1, m_FatFile);
}

// Načte fat tabulky
void Fat::loadFatTable() {
    assert(m_BootRecord != nullptr);

    m_fatTables = new unsigned int *[m_BootRecord->fat_copies];
    std::fseek(m_FatFile, getFatStartIndex(), SEEK_SET);

    unsigned int *fat_item = new unsigned int;
    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        m_fatTables[i] = new unsigned int[m_BootRecord->cluster_count];
        for (int j = 0; j < m_BootRecord->cluster_count; ++j) {
            std::fread(fat_item, sizeof(*fat_item), 1, m_FatFile);
            m_fatTables[i][j] = *fat_item;
        }
    }

    delete fat_item;
}

// Načte root directory
std::vector<std::shared_ptr<root_directory>> Fat::loadRootDirectory(long t_offset) {
    assert(m_BootRecord != nullptr);

    std::fseek(m_FatFile, t_offset, SEEK_SET);
    auto rootDirectory = std::vector<std::shared_ptr<root_directory>>(m_BootRecord->root_directory_max_entries_count);

    for (int i = 0; i < m_BootRecord->root_directory_max_entries_count; i++) {
        auto file = std::make_shared<root_directory>();
        std::fread(&(*file), sizeof(struct root_directory), 1, m_FatFile);
    }

    return rootDirectory;
}

// Uloží boot record
void Fat::saveBootRecord() {
    assert(m_FatFile != nullptr);

    fwrite(&(*m_BootRecord), sizeof(*m_BootRecord), 1, m_FatFile);
}

// Uloží všechny fat tabulky
void Fat::saveFatTables() {
    assert(m_FatFile != nullptr);

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        for (int j = 0; j < m_BootRecord->cluster_count; ++j) {
            unsigned int fatValue = m_fatTables[i][j];
            fwrite(&fatValue, sizeof(fatValue), 1, m_FatFile);
        }
    }
}

// Uloží root directories
void Fat::saveRootDirectory() {
    assert(m_FatFile != nullptr);

    for (auto &&rootDirectory : m_RootDirectories) {
        fwrite(&(*rootDirectory), sizeof(*rootDirectory), 1, m_FatFile);
    }

    auto delta = m_BootRecord->root_directory_max_entries_count - m_RootDirectories.size();
    if (delta > 0) {
        auto savedSize = sizeof(boot_record) * m_RootDirectories.size(); // Velikost paměti, která už je uložená
        auto needSize = m_BootRecord->cluster_size - savedSize;

        char tmp[needSize];
        memset(&tmp, '\0', sizeof(tmp));
        fwrite(&tmp, sizeof(tmp), 1, m_FatFile);
    }
}


// Najde první root_directory, která odpovídá zadané cestě
std::shared_ptr<root_directory> Fat:: findFirstCluster(
        const std::vector<std::shared_ptr<root_directory>> &t_RootDirectory, const std::string &t_path) {
    auto separatorIndex = t_path.find(PATH_SEPARATOR);
    std::string fileName;

    if (separatorIndex == std::string::npos) {
        fileName = t_path;
    } else {
        fileName = t_path.substr(0, separatorIndex);
    }

    for (auto &&rootDirectory : t_RootDirectory) {
        std::string directoryName = rootDirectory->file_name;
        if (fileName == directoryName) { // Když najdu požadovaný soubor
            if (rootDirectory->file_type == FILE_TYPE_FILE) { // Jedná -li se o soubor, tak ho vrátím
                return rootDirectory;
            } else {
                return findFirstCluster(loadRootDirectory(getClusterStartIndex(1)), t_path.substr(separatorIndex + 1, t_path.size()));
            }
        }
    }

    throw std::runtime_error("File not found");
}


// Vrátí index, kde začíná definice fat tabulky
const long Fat::getFatStartIndex() {
    if (m_FatStartIndex == 0) {
        m_FatStartIndex = sizeof(boot_record);
    }

    return m_FatStartIndex;
}

// Vrátí index, kde začíná definice root directory
const long Fat::getRootDirectoryStartIndex() {
    if (m_RootDirectoryStartIndex == 0) {
        m_RootDirectoryStartIndex = getFatStartIndex() + ((sizeof(unsigned int)) * m_BootRecord->cluster_count) * m_BootRecord->fat_copies;
    }

    return m_RootDirectoryStartIndex;
}

// Vrátí index, kde začínají clustry
const long Fat::getClustersStartIndex() {
    if (m_ClustersStartIndex == 0) {
        m_ClustersStartIndex = getRootDirectoryStartIndex() + (sizeof(root_directory)) * m_BootRecord->root_directory_max_entries_count;
    }
    return m_ClustersStartIndex;
}

// Vrátí index, kde začíná obsah požadovaného clusteru
const long Fat::getClusterStartIndex(int offset) {
    return static_cast<int>(getClustersStartIndex()) + (offset) * m_BootRecord->cluster_size;
}

const unsigned int Fat::getFreeCluster() {
    for (unsigned int i = 0; i < m_BootRecord->cluster_count; ++i) {
        if (m_fatTables[0][i] == FAT_UNUSED) {
            return i;
        }
    }

    throw std::runtime_error("Not enough space.");
}
