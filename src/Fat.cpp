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

#include <cstring>
#include <exception>
#include <assert.h>
#include <iostream>
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
    m_RootFile = makeFile("/", "", 0, FILE_TYPE_DIRECTORY, static_cast<unsigned int>(getRootDirectoryStartIndex()));
    loadFatTable();
    m_RootDirectories = loadDirectory(getRootDirectoryStartIndex());
}

std::shared_ptr<root_directory> Fat::findFirstCluster(const std::string &t_path) {
    return findFirstCluster(m_RootFile, m_RootDirectories, t_path);
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
    auto root_dir = makeFile(std::string(t_addr), std::string("rwxrwxrwx"), m_BootRecord->cluster_size, FILE_TYPE_DIRECTORY, getFreeCluster());

    std::vector<std::shared_ptr<root_directory>> parentDirectoryContent = loadDirectory(parentDirectory->first_cluster);

    if (parentDirectoryContent.size() >= m_BootRecord->root_directory_max_entries_count) {
        throw std::runtime_error("Can not create file, folder is full");
    }

    parentDirectoryContent.push_back(root_dir);
    saveClusterWithFiles(parentDirectoryContent, parentDirectory->first_cluster);

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        m_fatTables[i][root_dir->first_cluster] = FILE_TYPE_DIRECTORY;
    }
    saveFatPiece(root_dir->first_cluster, FILE_TYPE_DIRECTORY);

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

    for (auto &rootDirectory : m_RootDirectories) {
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

    printf("-------------------------------------------------------- \n");
    printf("CLUSTERS CONTENT \n");
    printf("-------------------------------------------------------- \n");

    auto *p_cluster = new char[m_BootRecord->cluster_size];
    std::memset(p_cluster, '\0', sizeof(p_cluster));
    std::fseek(m_FatFile, static_cast<int>(getClustersStartIndex()), SEEK_SET);

    for (int i = 0; i < m_BootRecord->cluster_count; ++i) {
        std::fread(p_cluster, sizeof(char) * m_BootRecord->cluster_size, 1, m_FatFile);

        if (p_cluster[0] == '\0') {
            //printf("Cluster %d: skip\n", i);
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

    for(;;) {
        std::fseek(&(*m_FatFile), getClusterStartIndex(workingCluster), SEEK_SET);
        std::fread(p_cluster, sizeof(char) * m_BootRecord->cluster_size, 1, m_FatFile);
        int cluster = workingCluster;
        if (workingCluster == FAT_FILE_END) {
            break;
        }
        workingCluster = m_fatTables[0][workingCluster];
        printf("Cluster %d: %s\n", cluster, p_cluster);

    }

    delete[] p_cluster;
}

// Private methods

// Načte boot record
void Fat::loadBootRecord() {
    std::fseek(m_FatFile, 0, SEEK_SET);
    m_BootRecord = std::make_unique<boot_record>();

    std::fread(&(*m_BootRecord), sizeof(boot_record), 1, m_FatFile);
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
std::vector<std::shared_ptr<root_directory>> Fat::loadDirectory(long t_offset) {
    assert(m_BootRecord != nullptr);

    std::fseek(m_FatFile, t_offset, SEEK_SET);
    auto rootDirectory = std::vector<std::shared_ptr<root_directory>>();

    for (int i = 0; i < m_BootRecord->root_directory_max_entries_count; i++) {
        auto file = std::make_shared<root_directory>();
        char x[sizeof(root_directory)];
        std::fread(&x, sizeof(root_directory), 1, m_FatFile);
        std::memcpy(&(*file), &x, sizeof(root_directory));
        if(x[0] != '\0') {
            rootDirectory.push_back(file);
        }
        //auto readed = std::fread(&(*file), sizeof(root_directory), 1, m_FatFile);
//        if (readed > 1) {
//            rootDirectory.push_back(file);
//        }
    }

    return rootDirectory;
}

// Uloží boot record
void Fat::saveBootRecord() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::fseek(m_FatFile, 0, SEEK_SET);

    std::fwrite(&(*m_BootRecord), sizeof(boot_record), 1, m_FatFile);
}

// Uloží všechny fat tabulky
void Fat::saveFatTables() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::fseek(m_FatFile, getFatStartIndex(), SEEK_SET);

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        for (int j = 0; j < m_BootRecord->cluster_count; ++j) {
            unsigned int fatValue = m_fatTables[i][j];
            std::fwrite(&fatValue, sizeof(unsigned int), 1, m_FatFile);
        }
    }
}

// Uloží změnu fat tabulky
void Fat::saveFatPiece(long offset, unsigned int value) {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        std::fseek(m_FatFile,
                   i * m_BootRecord->cluster_count * sizeof(unsigned int) // Offset podle kopie fat tabulek
                   + offset * sizeof(unsigned int), // Offset v jedné fatce
                   SEEK_SET);
        std::fwrite(&value, sizeof(unsigned int), 1, m_FatFile);
    }
}

// Uloží root directories
void Fat::saveRootDirectory() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    saveClusterWithFiles(m_RootDirectories, getRootDirectoryStartIndex());
}

// Uloží změnu v jednom clusteru se soubory
void Fat::saveClusterWithFiles(std::vector<std::shared_ptr<root_directory>> t_RootDirectory, long offset) {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::fseek(m_FatFile, offset, SEEK_SET);

    for (auto rootDirectory : t_RootDirectory) {
        std::fwrite(&(*rootDirectory), sizeof(root_directory), 1, m_FatFile);
    }
}

// Vytvoří nový soubor
std::shared_ptr<root_directory>
Fat::makeFile(std::string &&fileName, std::string &&fileMod, long fileSize, short fileType, unsigned int firstCluster) {
    auto file = std::make_shared<root_directory>();

    memset(file->file_mod, '\0', sizeof(file->file_mod));
    memset(file->file_name, '\0', sizeof(file->file_name));
    strcpy(file->file_mod, fileMod.c_str());
    strcpy(file->file_name, fileName.c_str());
    file->file_size = fileSize;
    file->file_type = fileType;
    file->first_cluster = firstCluster;

    return file;
}


// Najde první root_directory, která odpovídá zadané cestě
std::shared_ptr<root_directory> Fat::findFirstCluster(const std::shared_ptr<root_directory> &t_Parent,
        const std::vector<std::shared_ptr<root_directory>> &t_RootDirectory, const std::string &t_path) {
    auto separatorIndex = t_path.find(PATH_SEPARATOR);
    std::string fileName;

    if (separatorIndex == std::string::npos) {
        fileName = t_path;
    } else {
        fileName = t_path.substr(separatorIndex + 1);
    }

    for (auto &rootDirectory : t_RootDirectory) {
        std::string directoryName = rootDirectory->file_name;
        if (fileName == directoryName) { // Když najdu požadovaný soubor
            if (rootDirectory->file_type == FILE_TYPE_FILE) { // Jedná -li se o soubor, tak ho vrátím
                return rootDirectory;
            } else {
                return findFirstCluster(rootDirectory, loadDirectory(getClusterStartIndex(1)), t_path.substr(separatorIndex + 1, t_path.size()));
            }
        }
    }

    return t_Parent;
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
        m_RootDirectoryStartIndex = getFatStartIndex()
                                    + ((sizeof(unsigned int)) * m_BootRecord->cluster_count) * m_BootRecord->fat_copies;
    }

    return m_RootDirectoryStartIndex;
}

// Vrátí index, kde začínají clustry
const long Fat::getClustersStartIndex() {
    if (m_ClustersStartIndex == 0) {
        m_ClustersStartIndex = getRootDirectoryStartIndex()
                               + (sizeof(root_directory)) * m_BootRecord->root_directory_max_entries_count;
    }
    return m_ClustersStartIndex;
}

// Vrátí index, kde začíná obsah požadovaného clusteru
const long Fat::getClusterStartIndex(unsigned int offset) {
    return getClustersStartIndex() + offset * m_BootRecord->cluster_size;
}

const unsigned int Fat::getFreeCluster() {
    for (unsigned int i = 0; i < m_BootRecord->cluster_count; ++i) {
        if (m_fatTables[0][i] == FAT_UNUSED) {
            return i;
        }
    }

    throw std::runtime_error("Not enough space.");
}

