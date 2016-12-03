#include <assert.h>
#include "Fat.hpp"


Fat::Fat(std::string &t_filePath) : mFilePath(t_filePath) {}

// Public methods

// Načte fatku ze souboru
void Fat::loadFat() {
    m_fatFile.reset(fopen(mFilePath.c_str(), "r"));
    loadBootRecord();
    loadFatTable();
    loadRootDirectories();
    fclose(&(*m_fatFile));
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


// Private methods

// Načte boot record
void Fat::loadBootRecord() {
    std::fseek(&(*m_fatFile), 0, SEEK_SET);
    mBootRecord = std::make_unique<boot_record>();

    std::fread(&(*mBootRecord), sizeof(struct boot_record), 1, &(*m_fatFile));

    mFatTable = std::unique_ptr<int[]>(new int[mBootRecord->cluster_count]);
    for (int i = 0; i < mBootRecord->cluster_count; ++i) {
        mFatTable[i] = FAT_UNUSED;
    }
}

// Načte fat tabulky
void Fat::loadFatTable() {
    assert(mBootRecord != nullptr);

    std::fseek(&(*m_fatFile), getFatStartIndex(), SEEK_SET);

    unsigned int *fat_item;
    fat_item = (unsigned int *) malloc(sizeof(unsigned int));
    for (int i = 0; i < mBootRecord->cluster_count; i++) {
        fread(fat_item, sizeof(*fat_item), 1, &(*m_fatFile));
        mFatTable[i] = *fat_item;
    }

    for (int i = 0; i < mBootRecord->fat_copies - 1; ++i) {
        fread(fat_item, sizeof(*fat_item), 1, &(*m_fatFile));
    }

    free(fat_item);
}


// Načte root directories
void Fat::loadRootDirectories() {
    assert(mBootRecord != nullptr);

    std::fseek(&(*m_fatFile), getRootDirectoryStartIndex(), SEEK_SET);
    mRoot_directories = std::vector<std::shared_ptr<root_directory>>(mBootRecord->root_directory_max_entries_count);

    for (int i = 0; i < mBootRecord->root_directory_max_entries_count; i++) {
        auto rootDirectory = std::make_shared<root_directory>();
        fread(&(*rootDirectory), sizeof(struct root_directory), 1, &(*m_fatFile));

        mRoot_directories[i] = rootDirectory;
    }
}

// Vrátí index, kde začíná definice fat tabulky
long Fat::getFatStartIndex() {
    if (m_FatStartIndex == 0) {
        m_FatStartIndex = sizeof(boot_record);
    }

    return m_FatStartIndex;
}

// Vrátí index, kde začíná definice root directory
long Fat::getRootDirectoryStartIndex() {
    if (m_RootDirectoryStartIndex == 0) {
        m_RootDirectoryStartIndex = getFatStartIndex() + ((sizeof(unsigned int)) * mBootRecord->cluster_count) * mBootRecord->fat_copies;
    }

    return m_RootDirectoryStartIndex;
}

// Vrátí index, kde začínají clustry
long Fat::getClustersStartIndex() {
    if (m_ClustersStartIndex == 0) {
        m_ClustersStartIndex = getRootDirectoryStartIndex() + (sizeof(root_directory)) * mBootRecord->root_directory_max_entries_count;
    }
    return m_ClustersStartIndex;
}

long Fat::getClusterStartIndex(int offset) {
    return 0;
}
