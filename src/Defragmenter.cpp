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
#include <stack>
#include "Defragmenter.hpp"
#include "ThreadPool.hpp"


Defragmenter::Defragmenter(Fat &t_fat) : m_fat(t_fat) {
    m_translationTable = new unsigned int[m_fat.m_BootRecord->cluster_count];
    std::memcpy(m_translationTable, m_fat.m_fatTables[0], sizeof(m_fat.m_fatTables[0]));

    m_rootEntry = std::make_shared<file_entry>();
    m_rootEntry->me = m_fat.m_RootFile;

    for (auto &rootDirectory :m_fat.m_RootDirectories) {
        auto rootEntry = std::make_shared<file_entry>();
        rootEntry->parent = m_rootEntry;
        rootEntry->me = rootDirectory;
        m_rootEntry->children.push_back(rootEntry);
    }

    loadFullTree();
}

Defragmenter::~Defragmenter() {
    delete[] m_translationTable;
}

// Spustí defragmentaci fatky
void Defragmenter::runDefragmentation() {
    analyze();
}

// Nechá fatku vypsat stromovu strukturu, kde jsou vidět čísla clusterů
void Defragmenter::printTree() {
    std::printf("+%s\n", m_rootEntry->me->file_name);

    for (auto &item : m_rootEntry->children) {
        printSubTree(item, Fat::SPACE_SIZE);
    }
}

// Vypíše stromovou strukturu od rodiče
void Defragmenter::printSubTree(std::shared_ptr<file_entry> t_parent, unsigned int t_depth) {
    auto isDirectory = t_parent->me->file_type == Fat::FILE_TYPE_DIRECTORY;
    std::printf("%*s%s \n", t_depth, isDirectory ? "+" : "-", t_parent->me->file_name);

    for (auto &child :t_parent->children) {
        printSubTree(child, t_depth + Fat::SPACE_SIZE);
    }
}

// Sestaví úplnou cestu od souboru ke kořeni
std::string Defragmenter::getFullPath(std::shared_ptr<file_entry> t_file) {
    std::string path(t_file->me->file_name);

    auto parent = t_file->parent;
    while(std::strcmp(parent->me->file_name, m_fat.m_RootFile->file_name) != 0) {
        path.insert(0, PATH_SEPARATOR);
        path.insert(0, parent->me->file_name);
        parent = parent->parent;
    }

    path.insert(0, PATH_SEPARATOR);

    return path;
}

// Načte celou stromovou strukturu
void Defragmenter::loadFullTree() {
    loadSubTree(m_rootEntry);
}

// Načte stromovou strukturu j
void Defragmenter::loadSubTree(std::shared_ptr<file_entry> t_parent) {
    std::vector<ThreadPool::TaskFuture<void>> vector;
    for (auto &child :t_parent->children) {
        if (child->me->file_type != Fat::FILE_TYPE_DIRECTORY) {
            continue;
        }

        auto content = m_fat.loadDirectory(child->me->first_cluster);
        std::sort(content.begin(), content.end(), [](std::shared_ptr<root_directory> lhs,
                                                     std::shared_ptr<root_directory> rhs) {
            if ((lhs->file_type == Fat::FILE_TYPE_DIRECTORY && rhs->file_type == Fat::FILE_TYPE_DIRECTORY)
                    || (lhs->file_type != Fat::FILE_TYPE_DIRECTORY && rhs->file_type != Fat::FILE_TYPE_DIRECTORY)) {
                std::strcmp(lhs->file_name, rhs->file_name);
            }

            if (lhs->file_type != Fat::FILE_TYPE_DIRECTORY) {
                return 1;
            }

            if (rhs->file_type != Fat::FILE_TYPE_DIRECTORY) {
                return -1;
            }

            return std::strcmp(lhs->file_name, rhs->file_name);
        });
        for (auto &itemContent : content) {
            auto rootEntry = std::make_shared<file_entry>();
            rootEntry->me = itemContent;
            rootEntry->parent = child;
            child->children.push_back(rootEntry);
        }

        if (DefaultThreadPool::hasIdleThreads()) {
        DefaultThreadPool::submitJob([&](std::shared_ptr<file_entry> fileEntry) {
            loadSubTree(fileEntry);
        }, child);
        } else {
            loadSubTree(child);
        }
    }

    for (auto &&item : vector) {
        item.get();
    }
}

// Analyzuje fat tabulku a vytvoří transakční žurnál
void Defragmenter::analyze() {
    unsigned int actualClusterIndex = 1;


    std::queue<std::shared_ptr<file_entry>> workingStack;
    for(auto it = m_rootEntry->children.rbegin(); it != m_rootEntry->children.rend(); ++it) {
        workingStack.push(*it);
    }

    while(!workingStack.empty()) {
        auto actual = std::move(workingStack.front());
        workingStack.pop();

        if (actual->me->file_type == Fat::FILE_TYPE_DIRECTORY) { // Jedná-li se o složku, přidám její obsah na konec zásobníku
            for(auto it = actual->children.rbegin(); it != actual->children.rend(); ++it) {
                workingStack.push(*it);
            }
        } else { // Nejedná-li se o složku, analyzuji soubor
            auto clusters = m_fat.getClusters(actual->me);
            auto badClusterIndex = needReplace(clusters);
            if (badClusterIndex == 0) {
                continue;
            }
            std::printf("Soubor %s potřebuje transfer\n", getFullPath(actual).c_str());

            auto lastGoodClusterIndex = badClusterIndex - 1;
            auto goodCluster = clusters.at(lastGoodClusterIndex);
            for(unsigned int i = badClusterIndex; i < clusters.size(); i++) {
                const auto &clusterToReplace = clusters.at(i);
                const auto newCluster = goodCluster + 1;

                std::printf("Přesouvám cluster %d na novou pozici: %d\n", clusterToReplace, newCluster);
                goodCluster++;
            }
        }
    }
}

// Zjistí, zda-li je potřeba shluknout obsah souboru. Pokud ano, vrátí index prvního clusteru, který musí být upraven
unsigned int Defragmenter::needReplace(std::vector<unsigned int> &clusters) {
    if (clusters.empty()) {
        return 0;
    }

    auto index = clusters.front();
    for(unsigned int i = 0; i < clusters.size(); i++) {
        const auto &cluster = clusters.at(i);
        if (index != cluster) {
            return i;
        }

        index++;
    }

    return 0;
}

// Aplikuje transakce z transakčního žurnálu na celý souborov systém
void Defragmenter::applyTransactions() {

}

