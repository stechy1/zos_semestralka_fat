#include <assert.h>
#include <cstring>
#include <exception>
#include "Fat.hpp"


Fat::Fat(std::string &t_filePath) : mFilePath(t_filePath) {}

// Public methods

// Načte fatku ze souboru
void Fat::loadFat() {
    m_fatFile.reset(fopen(mFilePath.c_str(), "r"));
    loadBootRecord();
    loadFatTable();
    mRoot_directories = loadRootDirectories(getRootDirectoryStartIndex());
    //fclose(&(*m_fatFile));
}

std::shared_ptr<root_directory> Fat::findFirstCluster(const std::string &path) {
    return findFirstCluster(mRoot_directories, path);
}


// Print methods

// Vypíše obsah boot recordu
void Fat::printBootRecord() {
    assert(mBootRecord != nullptr);

    printf("-------------------------------------------------------- \n");
    printf("BOOT RECORD \n");
    printf("-------------------------------------------------------- \n");
    printf("volume_descriptor :%s\n",mBootRecord->volume_descriptor);
    printf("fat_type :%d\n",mBootRecord->fat_type);
    printf("fat_copies :%d\n",mBootRecord->fat_copies);
    printf("cluster_size :%d\n",mBootRecord->cluster_size);
    printf("root_directory_max_entries_count :%ld\n",mBootRecord->root_directory_max_entries_count);
    printf("cluster count :%d\n",mBootRecord->cluster_count);
    printf("reserved clusters :%d\n",mBootRecord->reserved_cluster_count);
    printf("signature :%s\n",mBootRecord->signature);
}

// Vypíše všechny root directories
void Fat::printRootDirectories() {
    printf("-------------------------------------------------------- \n");
    printf("ROOT DIRECTORY \n");
    printf("-------------------------------------------------------- \n");

    for (auto &&root_directory : mRoot_directories) {
        printRootDirectory(root_directory);
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
    assert(m_fatFile != nullptr);
    assert(mBootRecord != nullptr);

    auto *p_cluster = new char[mBootRecord->cluster_size];
    std::memset(p_cluster, '\0', sizeof(p_cluster));
    std::fseek(&(*m_fatFile), static_cast<int>(getClustersStartIndex()), SEEK_SET);

    for (int i = 0; i < mBootRecord->cluster_count; ++i) {
        std::fread(p_cluster, sizeof(char) * mBootRecord->cluster_size, 1, &(*m_fatFile));

        if (p_cluster[0] == '\0') {
            printf("Cluster %d: skip\n", i);
            continue;
        }

        printf("Cluster %d: %s\n", i, p_cluster);
    }

    delete[] p_cluster;
}

// Vypíše obsah celého souboru
void Fat::printFileContent(std::shared_ptr<root_directory> t_rootDirectory) {
    assert(m_fatFile != nullptr);
    assert(mBootRecord != nullptr);

    auto *p_cluster = new char[mBootRecord->cluster_size];
    std::memset(p_cluster, '\0', sizeof(p_cluster));
    auto workingCluster = t_rootDirectory->first_cluster;
    bool isEOF;

    do {
        std::fseek(&(*m_fatFile), getClusterStartIndex(workingCluster), SEEK_SET);

        std::fread(p_cluster, sizeof(char) * mBootRecord->cluster_size, 1, &(*m_fatFile));
        printf("Cluster %d: %s\n", workingCluster, p_cluster);
        workingCluster = mFatTable[workingCluster];
        isEOF = workingCluster == FAT_FILE_END;

    } while (!isEOF);

    delete[] p_cluster;
}

// Private methods

// Načte boot record
void Fat::loadBootRecord() {
    std::fseek(&(*m_fatFile), 0, SEEK_SET);
    mBootRecord = std::make_unique<boot_record>();

    std::fread(&(*mBootRecord), sizeof(struct boot_record), 1, &(*m_fatFile));

    mFatTable = std::unique_ptr<unsigned int[]>(new unsigned int[mBootRecord->cluster_count]);
    for (int i = 0; i < mBootRecord->cluster_count; ++i) {
        mFatTable[i] = FAT_UNUSED;
    }
}

// Načte fat tabulky
void Fat::loadFatTable() {
    assert(mBootRecord != nullptr);

    std::fseek(&(*m_fatFile), getFatStartIndex(), SEEK_SET);

    unsigned int *fat_item = new unsigned int;
    for (int i = 0; i < mBootRecord->cluster_count; i++) {
        std::fread(fat_item, sizeof(*fat_item), 1, &(*m_fatFile));
        mFatTable[i] = *fat_item;
    }

    for (int i = 0; i < mBootRecord->fat_copies - 1; ++i) {
        std::fread(fat_item, sizeof(*fat_item), 1, &(*m_fatFile));
    }

    delete fat_item;
}

// Načte root directories
std::vector<std::shared_ptr<root_directory>> Fat::loadRootDirectories(long offset) {
    assert(mBootRecord != nullptr);

    std::fseek(&(*m_fatFile), offset, SEEK_SET);
    auto rootDirectories = std::vector<std::shared_ptr<root_directory>>(mBootRecord->root_directory_max_entries_count);

    for (int i = 0; i < mBootRecord->root_directory_max_entries_count; i++) {
        auto rootDirectory = std::make_shared<root_directory>();
        std::fread(&(*rootDirectory), sizeof(struct root_directory), 1, &(*m_fatFile));

        rootDirectories[i] = rootDirectory;
    }

    return rootDirectories;
}

// Najde první root_directory, která odpovídá zadané cestě
std::shared_ptr<root_directory> Fat:: findFirstCluster(
        const std::vector<std::shared_ptr<root_directory>> &t_Root_directories, const std::string &path) {
    auto separatorIndex = path.find(PATH_SEPARATOR);
    std::string fileName;

    if (separatorIndex == std::string::npos) {
        fileName = path;
    } else {
        fileName = path.substr(0, separatorIndex);
    }

    for (auto &&rootDirectory : t_Root_directories) {
        std::string directoryName = rootDirectory->file_name;
        if (fileName == directoryName) { // Když najdu požadovaný soubor
            if (rootDirectory->file_type == FILE_TYPE_FILE) { // Jedná -li se o soubor, tak ho vrátím
                return rootDirectory;
            } else {
                return findFirstCluster(loadRootDirectories(getClusterStartIndex(1)), path.substr(separatorIndex + 1, path.size()));
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
        m_RootDirectoryStartIndex = getFatStartIndex() + ((sizeof(unsigned int)) * mBootRecord->cluster_count) * mBootRecord->fat_copies;
    }

    return m_RootDirectoryStartIndex;
}

// Vrátí index, kde začínají clustry
const long Fat::getClustersStartIndex() {
    if (m_ClustersStartIndex == 0) {
        m_ClustersStartIndex = getRootDirectoryStartIndex() + (sizeof(root_directory)) * mBootRecord->root_directory_max_entries_count;
    }
    return m_ClustersStartIndex;
}

// Vrátí index, kde začíná obsah požadovaného clusteru
const long Fat::getClusterStartIndex(int offset) {
    return static_cast<int>(getClustersStartIndex()) + (offset) * mBootRecord->cluster_size;
}

