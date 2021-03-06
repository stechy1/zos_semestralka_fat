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
#include <stack>
#include <unistd.h>
#include <thread>
#include "Fat.hpp"


Fat::Fat(std::string &t_filePath) : m_FilePath(t_filePath) {
    openFile();

    loadFat();
}

Fat::~Fat() {

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        delete[] m_fatTables[i];
    }

    delete[] m_fatTables;
    m_workingFatTable = nullptr;

    closeFile();
}


// Public methods

// Načte fatku ze souboru
void Fat::loadFat() {
    try {
        loadBootRecord();
        m_RootFile = makeFile("/", "", 0, FILE_TYPE_DIRECTORY, 0);
        loadFatTable();
        m_RootDirectories = loadDirectory(0);
    } catch (std::exception &ex) {
        std::printf("%s", ex.what());
    }
}

// Vrátí referenci na root_directory obsahující informace o souboru/složce
std::shared_ptr<root_directory> Fat::findFileDescriptor(const std::string &t_path) {
    return findFileDescriptor(m_RootFile, m_RootDirectories, t_path);
}

// Vrátí vektor clusterů, které tvoří obsah souboru
std::vector<unsigned int> Fat::getClusters(std::shared_ptr<root_directory> t_fileEntry) {
    assert(m_BootRecord != nullptr);
    assert(m_workingFatTable != nullptr);

    auto clusters = std::vector<unsigned int>();
    if (t_fileEntry->file_type == FILE_TYPE_DIRECTORY) {
        return clusters;
    }

    auto clusterSize = m_BootRecord->cluster_size;
    auto fileSize = t_fileEntry->file_size;
    auto maxIterationCount = fileSize % clusterSize;
    auto workingCluster = t_fileEntry->first_cluster;
    bool isEOF;
    auto count = 0;

    do {
        if (count >= maxIterationCount && t_fileEntry->file_type != FILE_TYPE_DIRECTORY) { // Detekce zacyklení
            throw std::runtime_error("Inconsistent fat table");
        }
        clusters.push_back(workingCluster);
        workingCluster = m_workingFatTable[workingCluster];
        isEOF = workingCluster == FAT_FILE_END;
        count++;
    } while (!isEOF);

    return clusters;
}

// Vypíše stromovou strukturu celé fatky
void Fat::tree() {
    assert(m_BootRecord != nullptr);
    assert(m_FatFile != nullptr);

    std::printf("+%s\n", m_RootFile->file_name);

    for (auto &fileEntry : m_RootDirectories) {
        printSubTree(fileEntry, SPACE_SIZE);
    }
}

// Vytvoří nový adresář
void Fat::createDirectory(const std::string &t_path, const std::string &t_addr) {
    auto parentDirectory = findFileDescriptor(t_path);
    auto directory = makeFile(t_addr, "rwxrwxrwx", m_BootRecord->cluster_size, FILE_TYPE_DIRECTORY, getFreeCluster());

    std::vector<std::shared_ptr<root_directory>> parentDirectoryContent = loadDirectory(parentDirectory->first_cluster);

    if (parentDirectoryContent.size() >= m_BootRecord->root_directory_max_entries_count) {
        throw std::runtime_error("Can not create file, folder is full");
    }

    for (auto &&item : parentDirectoryContent) {
        if(std::strcmp(item->file_name, directory->file_name) == 0) {
            throw std::runtime_error("File already exists");
        }
    }

    parentDirectoryContent.push_back(directory);
    saveClusterWithFiles(parentDirectoryContent, parentDirectory->first_cluster);

    setFatPiece(directory->first_cluster, FAT_DIRECTORY_CONTENT);
    saveFatPiece(directory->first_cluster, FAT_DIRECTORY_CONTENT);
}

// Smaže aadresář
void Fat::deleteDirectory(const std::string &t_pseudoPath) {
    auto separatorIndex = t_pseudoPath.find_last_of(PATH_SEPARATOR);
    auto parentDirectoryPath = t_pseudoPath;
    auto fileName = t_pseudoPath;
    if (separatorIndex != std::string::npos) {
        parentDirectoryPath = t_pseudoPath.substr(0, separatorIndex);
        fileName = t_pseudoPath.substr(separatorIndex + 1);
    }
    auto parentDirectory = findFileDescriptor(parentDirectoryPath); // Rodičovský adresář
    auto parentDirectoryContent = loadDirectory(parentDirectory->first_cluster); // Načte obsah rodičovského adresáře
    auto fileEntry = findFileDescriptor(t_pseudoPath); // Vlastní soubor
    auto directoryContent = loadDirectory(fileEntry->first_cluster);

    if (directoryContent.size() != 0) {
        throw std::runtime_error("Path not empty");
    }

    int deleteIndex = -1;
    for (auto &content : parentDirectoryContent) {
        if (fileName == content->file_name) {
            deleteIndex++;
            break;
        }

        deleteIndex++;
    }

    if (deleteIndex == -1) {
        throw std::runtime_error("File not found");
    }

    parentDirectoryContent.erase(parentDirectoryContent.begin() + deleteIndex);
    saveClusterWithFiles(parentDirectoryContent, parentDirectory->first_cluster);

    clearFatRecord(fileEntry->first_cluster);
}

// Vytvoří novou čistou fatku
void Fat::createEmptyFat() {
    closeFile();
    unlink(m_FilePath.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    openFile();
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
    m_workingFatTable = m_fatTables[0];

    setFatPiece(0, FAT_FILE_END);

    saveBootRecord();
    saveFatTables();

    std::fseek(m_FatFile, getClustersStartIndex(), SEEK_SET);
    char tmp[m_BootRecord->cluster_size];
    std::memset(&tmp, '\0', sizeof(tmp));
    for (int i = 0; i < m_BootRecord->cluster_count; ++i) {
        std::fwrite(&tmp, sizeof(tmp), 1, m_FatFile);
    }

    if (m_BootRecord->root_directory_max_entries_count * sizeof(root_directory) > CLUSTER_SIZE) {
        std::printf("Velikost clusteru není dostatečná, aby pokryla maximální počet souborů/složek.\n");
        std::printf("Hrozí přepisování obsahu clusterů.\n");
        std::printf("Zvětšete parametr 'CLUSTER_SIZE', nebo zmenšete parametr 'root_directory_max_entries_count'.\n");
    }
}

// Vloží soubor
void Fat::insertFile(const std::string &t_filePath, const std::string &t_pseudoPath) {
    FILE *workingFile = std::fopen(t_filePath.c_str(), "r");

    if (workingFile == nullptr) {
        throw std::runtime_error("File not found");
    }

    auto separatorIndex = t_pseudoPath.find_last_of(PATH_SEPARATOR);
    auto realPseudoPath = t_pseudoPath.substr(0, separatorIndex);
    auto parentDirectory = findFileDescriptor(realPseudoPath);
    auto parentDirectoryContent = loadDirectory(parentDirectory->first_cluster);
    auto fileName = t_pseudoPath.substr(separatorIndex + 1);

    std::fseek(workingFile, 0L, SEEK_END);

    auto fileSize = std::ftell(workingFile);
    auto file = makeFile(fileName, "rwxrwxrwx", fileSize, FILE_TYPE_FILE, getFreeCluster(FAT_FIRST_CONTENT_INDEX));

    if (parentDirectoryContent.size() >= m_BootRecord->root_directory_max_entries_count) {
        throw std::runtime_error("Can not create file, folder is full");
    }

    for (auto &&item : parentDirectoryContent) {
        if(std::strcmp(item->file_name, file->file_name) == 0) {
            throw std::runtime_error("File already exists");
        }
    }

    writeFile(workingFile, file);

    parentDirectoryContent.push_back(file);
    saveClusterWithFiles(parentDirectoryContent, parentDirectory->first_cluster);

    std::fclose(workingFile);
}

// Smaže soubor
void Fat::deleteFile(const std::string &t_pseudoPath) {
    auto separatorIndex = t_pseudoPath.find_last_of(PATH_SEPARATOR);
    auto parentDirectoryPath = t_pseudoPath;
    auto fileName = t_pseudoPath;
    if (separatorIndex != std::string::npos) {
        parentDirectoryPath = t_pseudoPath.substr(0, separatorIndex);
        fileName = t_pseudoPath.substr(separatorIndex + 1);
    }
    auto parentDirectory = findFileDescriptor(parentDirectoryPath); // Rodičovský adresář
    auto parentDirectoryContent = loadDirectory(parentDirectory->first_cluster); // Načte obsah rodičovského adresáře
    auto fileEntry = findFileDescriptor(t_pseudoPath); // Vlastní soubor

    int deleteIndex = -1;
    bool found = false;
    for (auto &content : parentDirectoryContent) {
        if (fileName == content->file_name) {
            deleteIndex++; // Protože začínám od hodnoty -1
            found = true;
            break;
        }

        deleteIndex++;
    }

    if (!found) {
        throw std::runtime_error("File not found");
    }

    parentDirectoryContent.erase(parentDirectoryContent.begin() + deleteIndex);
    saveClusterWithFiles(parentDirectoryContent, parentDirectory->first_cluster);

    clearFatRecord(fileEntry->first_cluster);
}

// Vrátí surový obsah
const std::string Fat::getClusterContent(const unsigned int t_index) {
    assert(m_BootRecord != nullptr);
    assert(m_workingFatTable != nullptr);

    std::fseek(m_FatFile, getClusterStartIndex(t_index), SEEK_SET);
    char tmp[m_BootRecord->cluster_size];
    std::memset(&tmp, '\0', sizeof(tmp));
    std::fread(&tmp, sizeof(tmp), 1, m_FatFile);

    return std::string(tmp);
}

// Vrátí obsah souboru ve vektoru
const std::vector<std::string> Fat::readFileContent(const std::shared_ptr<root_directory> t_rootDirectory) {
    return readFileContent(t_rootDirectory->first_cluster, t_rootDirectory->file_size);
}

// Vrátí obsah souboru ve vektoru
const std::vector<std::string> Fat::readFileContent(const unsigned int t_index, const long t_fileSize) {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::vector<std::string> result;
    auto clusterSize = m_BootRecord->cluster_size;
    char p_cluster[clusterSize];
    std::memset(p_cluster, '\0', sizeof(p_cluster));
    auto workingCluster = t_index;
    auto maxIterationCount = t_fileSize % clusterSize;
    auto count = 0;

    for (;;) {
        if (count >= maxIterationCount) { // Detekce zacyklení
            throw std::runtime_error("Inconsistent fat table");
        }
        std::fseek(&(*m_FatFile), getClusterStartIndex(workingCluster), SEEK_SET);
        std::fread(&p_cluster, clusterSize, 1, m_FatFile);
        if (workingCluster == FAT_FILE_END) {
            break;
        }
        workingCluster = m_workingFatTable[workingCluster];
        result.push_back(std::string(p_cluster));
        count++;
    }

    return result;
}

// Zapíše surový obsah do clusteru
void Fat::writeClusterContent(const unsigned int t_index, const std::string &t_data) {
    assert(m_BootRecord != nullptr);
    assert(m_workingFatTable != nullptr);

    std::fseek(m_FatFile, getClusterStartIndex(t_index), SEEK_SET);
    std::fwrite(t_data.c_str(), t_data.size(), 1, m_FatFile);
}

// Uloží statické informace o fatce do souboru
void Fat::save() {
    saveBootRecord();
    saveFatTables();
    saveRootDirectory();
}

// Print methods

// Vypíše obsah boot recordu
void Fat::printBootRecord() {
    assert(m_BootRecord != nullptr);

    std::printf("-------------------------------------------------------- \n");
    std::printf("BOOT RECORD \n");
    std::printf("-------------------------------------------------------- \n");
    std::printf("volume_descriptor :%s\n", m_BootRecord->volume_descriptor);
    std::printf("fat_type :%d\n", m_BootRecord->fat_type);
    std::printf("fat_copies :%d\n", m_BootRecord->fat_copies);
    std::printf("cluster_size :%d\n", m_BootRecord->cluster_size);
    std::printf("root_directory_max_entries_count :%ld\n", m_BootRecord->root_directory_max_entries_count);
    std::printf("cluster count :%d\n", m_BootRecord->cluster_count);
    std::printf("reserved content :%d\n", m_BootRecord->reserved_cluster_count);
    std::printf("signature :%s\n", m_BootRecord->signature);
}

// Vypíše všechny root directories
void Fat::printRootDirectories() {
    std::printf("-------------------------------------------------------- \n");
    std::printf("ROOT DIRECTORY \n");
    std::printf("-------------------------------------------------------- \n");

    for (auto &rootDirectory : m_RootDirectories) {
        printRootDirectory(rootDirectory);
    }
}

// Vypíše obsah požadované boot directory
void Fat::printRootDirectory(const std::shared_ptr<root_directory> t_rootDirectory) {
    std::printf("file_name :%s\n", t_rootDirectory->file_name);
    std::printf("file_mod :%s\n", t_rootDirectory->file_mod);
    std::printf("file_type :%d\n", t_rootDirectory->file_type);
    std::printf("file_size :%li\n", t_rootDirectory->file_size);
    std::printf("first_cluster :%d\n", t_rootDirectory->first_cluster);
}

// Vypíše obsahy clusterů, které obsahují soubory
void Fat::printClustersContent() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    std::printf("-------------------------------------------------------- \n");
    std::printf("CLUSTERS CONTENT \n");
    std::printf("-------------------------------------------------------- \n");

    char p_cluster[m_BootRecord->cluster_size];
    std::fseek(m_FatFile, static_cast<int>(getClusterStartIndex(FAT_FIRST_CONTENT_INDEX)), SEEK_SET);

    for (int i = FAT_FIRST_CONTENT_INDEX; i < m_BootRecord->cluster_count; ++i) {
        std::memset(p_cluster, '\0', sizeof(p_cluster));
        std::fread(p_cluster, sizeof(char) * m_BootRecord->cluster_size, 1, m_FatFile);

        if (m_workingFatTable[i] == FAT_BAD_CLUSTER || m_workingFatTable[i] == FAT_UNUSED || m_workingFatTable[i] == FAT_DIRECTORY_CONTENT) {
            continue;
        }

        if (p_cluster[0] == '\0') {
            continue;
        }

        std::printf("Cluster %d: %s\n", i, p_cluster);
    }
}

// Vypíše obsah celého souboru
void Fat::printFileContent(const std::vector<std::string> &t_fileContent) {
    for (auto &&content : t_fileContent) {
        std::printf("%s\n", content.c_str());
    }
}


// Vypíše stromovou strukturu od zadaného adresáře
void Fat::printSubTree(const std::shared_ptr<root_directory> t_rootDirectory, const unsigned int t_depth) {
    bool isDirectory = t_rootDirectory->file_type == FILE_TYPE_DIRECTORY;
    std::printf("%*s%s ", t_depth, isDirectory ? "+" : "-",
                t_rootDirectory->file_name); // vypíše tabulátor podle aktuálního zanoření a "+" - directory, jinak "-"
    auto content = loadDirectory(t_rootDirectory->first_cluster);
    auto workingDirectory = t_rootDirectory->first_cluster;
    auto maxIterationCount = t_rootDirectory->file_size % m_BootRecord->cluster_size;
    int count = 0;

    if (isDirectory) { // Pokud se jedná o složku, tak maximální počet iterací = počet souborů ve složce
        maxIterationCount = content.size();
    }
    while (1) { // Výpis clusterů
        std::printf("%d", workingDirectory);
        workingDirectory = m_workingFatTable[workingDirectory];
        count++;

        if (workingDirectory == FAT_UNUSED
            || workingDirectory == FAT_BAD_CLUSTER
            || workingDirectory == FAT_DIRECTORY_CONTENT
            || workingDirectory == FAT_FILE_END) {
            break;
        }
        if (count > maxIterationCount) { // Detekce zacyklení
            throw std::runtime_error("Inconsistent fat table");
        }
        std::printf(",");
    }
    std::printf(" (%d)\n", count);

    if (isDirectory) {
        auto nextDepth = t_depth + SPACE_SIZE;
        for (auto &fileEntry : content) {
            printSubTree(fileEntry, nextDepth);
        }
    }
}

// Private methods

// Otevře soubor s fatkou
void Fat::openFile() {
    m_FatFile = std::fopen(m_FilePath.c_str(), "r+");
    if (m_FatFile == 0) {
        m_FatFile = std::fopen(m_FilePath.c_str(), "w+");
    }
}

// Zavře soubor s fatkou
void Fat::closeFile() {
    std::fclose(m_FatFile);
}

// Načte boot record
void Fat::loadBootRecord() {
    assert(m_FatFile != nullptr);

    std::fseek(m_FatFile, 0, SEEK_SET);

    char tmp[sizeof(boot_record)];
    std::memset(&tmp, '\0', sizeof(boot_record));
    std::fread(&tmp, sizeof(boot_record), 1, m_FatFile);
    if (tmp[0] == '\0') {
        throw std::runtime_error("Fat is damaged");
    }

    m_BootRecord = std::make_unique<boot_record>();
    std::memcpy(&(*m_BootRecord), &tmp, sizeof(boot_record));
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
            std::fread(fat_item, sizeof(unsigned int), 1, m_FatFile);
            m_fatTables[i][j] = *fat_item;
        }
    }
    m_workingFatTable = m_fatTables[0];

    delete fat_item;
}

// Načte root directory
std::vector<std::shared_ptr<root_directory>> Fat::loadDirectory(unsigned int t_offset) {
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    std::fseek(m_FatFile, getClusterStartIndex(t_offset), SEEK_SET);
    auto rootDirectory = std::vector<std::shared_ptr<root_directory>>();

    for (int i = 0; i < m_BootRecord->root_directory_max_entries_count; i++) {
        auto file = std::make_shared<root_directory>();
        char x[sizeof(root_directory)];
        std::memset(&x, '\0', sizeof(root_directory));
        std::fread(&x, sizeof(root_directory), 1, m_FatFile);
        std::memcpy(&(*file), &x, sizeof(root_directory));
        if (x[0] != '\0') {
            rootDirectory.push_back(file);
        }
    }

    return rootDirectory;
}

// Uloží boot record
void Fat::saveBootRecord() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    std::fseek(m_FatFile, 0, SEEK_SET);

    std::fwrite(&(*m_BootRecord), sizeof(boot_record), 1, m_FatFile);
}

// Uloží všechny fat tabulky
void Fat::saveFatTables() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    std::fseek(m_FatFile, getFatStartIndex(), SEEK_SET);

    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        for (int j = 0; j < m_BootRecord->cluster_count; ++j) {
            unsigned int fatValue = m_fatTables[i][j];
            std::fwrite(&fatValue, sizeof(unsigned int), 1, m_FatFile);
        }
    }
}

// Uloží změnu fat tabulky
void Fat::saveFatPiece(long t_offset, unsigned int t_value) {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        std::fseek(m_FatFile,
                   getFatStartIndex()
                   + i * m_BootRecord->cluster_count * sizeof(unsigned int) // Offset podle kopie fat tabulek
                   + t_offset * sizeof(unsigned int), // Offset v jedné fatce
                   SEEK_SET);
        std::fwrite(&t_value, sizeof(unsigned int), 1, m_FatFile);
    }
}

// Uloží root directories
void Fat::saveRootDirectory() {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    saveClusterWithFiles(m_RootDirectories, 0);
}

// Uloží změnu v jednom clusteru se soubory
void Fat::saveClusterWithFiles(std::vector<std::shared_ptr<root_directory>> t_rootDirectory, unsigned int t_offset) {
    assert(m_FatFile != nullptr);
    assert(m_BootRecord != nullptr);

    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    auto clusterStartIndex = getClusterStartIndex(t_offset);
    auto clusterSize = m_BootRecord->cluster_size;

    std::fseek(m_FatFile, clusterStartIndex, SEEK_SET);
    char tmp[clusterSize];
    std::memset(&tmp, '\0', clusterSize);
    std::fwrite(&tmp, clusterSize, 1, m_FatFile);
    std::fseek(m_FatFile, clusterStartIndex, SEEK_SET);

    for (auto rootDirectory : t_rootDirectory) {
        char x[sizeof(root_directory)];
        std::memset(&x, '\0', sizeof(root_directory));
        std::memcpy(&x, &(*rootDirectory), sizeof(root_directory));
        std::fwrite(&(*rootDirectory), sizeof(root_directory), 1, m_FatFile);
    }
}

// Nastaví záznam v fat tabulce a jeji kopiích
void Fat::setFatPiece(long t_offset, unsigned int t_value) {
    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    for (int i = 0; i < m_BootRecord->fat_copies; ++i) {
        m_fatTables[i][t_offset] = t_value;
    }
}

// Vymaže záznam ze zadaného offsetu ve fat tabulce
void Fat::clearFatRecord(long t_offset) {
    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    auto workingOffset = t_offset;
    auto counter = 0;

    for(;;) {
        if (counter > m_BootRecord->cluster_count) {
            throw std::runtime_error("Fat is inconsistent");
        }

        if (workingOffset == FAT_FILE_END || workingOffset == FAT_BAD_CLUSTER ||
            workingOffset == FAT_DIRECTORY_CONTENT) {
            break;
        }

        auto nextOffset = m_workingFatTable[workingOffset];
        setFatPiece(workingOffset, FAT_UNUSED);
        workingOffset = nextOffset;
        counter++;
    }

    saveFatTables();
}

// Vytvoří nový soubor
std::shared_ptr<root_directory>
Fat::makeFile(const std::string &t_fileName, const std::string &t_fileMod, long t_fileSize, short t_fileType,
              unsigned int t_firstCluster) {
    auto file = std::make_shared<root_directory>();

    memset(file->file_mod, '\0', sizeof(file->file_mod));
    memset(file->file_name, '\0', sizeof(file->file_name));
    strcpy(file->file_mod, t_fileMod.c_str());
    strcpy(file->file_name, t_fileName.c_str());
    file->file_size = t_fileSize;
    file->file_type = t_fileType;
    file->first_cluster = t_firstCluster;

    return file;
}

// Najde první root_directory, která odpovídá zadané cestě
std::shared_ptr<root_directory> Fat::findFileDescriptor(const std::shared_ptr<root_directory> &t_parent,
                                                        const std::vector<std::shared_ptr<root_directory>> &t_rootDirectory,
                                                        const std::string &t_path) {
    std::unique_lock<std::recursive_mutex> lock(m_recursive_mutex);
    auto separatorIndex = t_path.find(PATH_SEPARATOR);
    std::string fileName;
    std::string wantedFileName;

    if (separatorIndex == std::string::npos) {
        fileName = t_path;
    } else {
        fileName = t_path.substr(separatorIndex + 1);
    }
    wantedFileName = fileName;

    auto nextSeparator = fileName.find(PATH_SEPARATOR);
    if (nextSeparator != std::string::npos) {
        wantedFileName = fileName.substr(0, nextSeparator);
    }
    for (auto &rootDirectory : t_rootDirectory) {
        std::string directoryName = rootDirectory->file_name;
        if (wantedFileName.compare(directoryName) == 0) { // Když najdu požadovaný soubor
            if (rootDirectory->file_type == FILE_TYPE_FILE) { // Jedná -li se o soubor, tak ho vrátím
                return rootDirectory;
            } else { // Jinak se zanořím rekurzivně o úroveň níž
                return findFileDescriptor(rootDirectory, loadDirectory(rootDirectory->first_cluster),
                                          t_path.substr(separatorIndex + 1));
            }
        }
    }

    if (t_path.find(PATH_SEPARATOR) != std::string::npos && !fileName.empty()) {
        throw std::runtime_error("File not found");
    }

    return t_parent;
}

// Zapíše obsah souboru do clusterů
void Fat::writeFile(FILE *t_file, std::shared_ptr<root_directory> t_fileEntry) {
    std::fseek(t_file, 0, SEEK_SET);
    auto fileSize = t_fileEntry->file_size;
    auto remaining = fileSize;
    auto workingCluster = t_fileEntry->first_cluster;
    auto clusterSize = m_BootRecord->cluster_size;
    auto needClusterCount = static_cast<unsigned int>(std::ceil(fileSize / static_cast<double>(clusterSize)));
    auto clusters = getFreeClusters(needClusterCount);
    clusters.erase(clusters.begin()); // Odstranění prvního indexu - ten uz je v t_file_entry->first_cluster
    char tmp[clusterSize];

    for (auto &&cluster : clusters) {
        auto realReadSize = remaining > clusterSize ? clusterSize : remaining;
        std::memset(&tmp, '\0', clusterSize);
        std::fseek(t_file, fileSize - remaining, SEEK_SET);
        std::fread(&tmp, static_cast<unsigned int>(realReadSize), 1, t_file);
        std::fseek(m_FatFile, getClusterStartIndex(workingCluster), SEEK_SET);
        std::fwrite(&tmp, clusterSize, 1, m_FatFile);

        std::printf("Zapisuji na cluster cislo: %d: %s\n", workingCluster, tmp);
        setFatPiece(workingCluster, cluster);
        workingCluster = cluster;
        remaining -= clusterSize;
    }
    setFatPiece(workingCluster, Fat::FAT_FILE_END);

    saveFatTables();
}

// Vrátí index, kde začíná definice fat tabulky
const unsigned int Fat::getFatStartIndex() {
    if (m_FatStartIndex == 0) {
        m_FatStartIndex = sizeof(boot_record);
    }

    return static_cast<unsigned int>(m_FatStartIndex);
}

// Vrátí index, kde začínají clustry
const unsigned int Fat::getClustersStartIndex() {
    if (m_ClustersStartIndex == 0) {
        m_ClustersStartIndex = getFatStartIndex()
                               + ((sizeof(unsigned int)) * m_BootRecord->cluster_count) * m_BootRecord->fat_copies;
    }
    return static_cast<unsigned int>(m_ClustersStartIndex);
}

// Vrátí index, kde začíná obsah požadovaného clusteru
const unsigned int Fat::getClusterStartIndex(unsigned int t_offset) {
    return getClustersStartIndex() + t_offset * m_BootRecord->cluster_size;
}

// Vrátí první volný cluster
const unsigned int Fat::getFreeCluster() {
    return getFreeCluster(FAT_FIRST_CONTENT_INDEX);
}

// Vrátí první volný cluster od zadaného offsetu
const unsigned int Fat::getFreeCluster(const unsigned int t_offset) {
    assert(t_offset < m_BootRecord->cluster_count);

    for (unsigned int i = t_offset; i < m_BootRecord->cluster_count; ++i) {
        if (m_workingFatTable[i] == FAT_UNUSED) {
            return i;
        }
    }

    throw std::runtime_error("Not enough space.");
}

// Vrátí vektor volných clusterů
const std::vector<unsigned int> Fat::getFreeClusters(const unsigned int t_count) {
    std::vector<unsigned int> result;
    unsigned int lastFoundIndex = FAT_FIRST_CONTENT_INDEX;

    for (int i = t_count; i >= 0; --i) {
        auto index = getFreeCluster(lastFoundIndex);
        result.push_back(index);
        lastFoundIndex = index + 1;
    }

    return result;
}
