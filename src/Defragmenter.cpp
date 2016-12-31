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
#include <assert.h>
#include "Defragmenter.hpp"
#include "ThreadPool.hpp"


Defragmenter::Defragmenter(Fat &t_fat) : m_fat(t_fat) {
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

Defragmenter::~Defragmenter() {}

// Spustí defragmentaci fatky
void Defragmenter::runDefragmentation() {
    for(;;) {
        auto success = analyze();
        if (success) {
            break;
        }
    }
    m_fat.save();
}

// Nechá fatku vypsat stromovu strukturu, kde jsou vidět čísla clusterů
void Defragmenter::printTree() {
    std::printf("+%s\n", m_rootEntry->me->file_name);

    for (auto &item : m_rootEntry->children) {
        printSubTree(item, Fat::SPACE_SIZE);
    }
}

// Vypíše stromovou strukturu od rodiče
void Defragmenter::printSubTree(const std::shared_ptr<file_entry> t_parent, const unsigned int t_depth) {
    auto isDirectory = t_parent->me->file_type == Fat::FILE_TYPE_DIRECTORY;
    std::printf("%*s%s \n", t_depth, isDirectory ? "+" : "-", t_parent->me->file_name);

    for (auto &child :t_parent->children) {
        printSubTree(child, t_depth + Fat::SPACE_SIZE);
    }
}

// Sestaví úplnou cestu od souboru ke kořeni
const std::string Defragmenter::getFullPath(const std::shared_ptr<file_entry> t_file) {
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

// Načte stromovou strukturu
void Defragmenter::loadSubTree(const std::shared_ptr<file_entry> t_parent) {
    std::vector<ThreadPool::TaskFuture<void>> vector;
    for (auto &child : t_parent->children) {
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
const bool Defragmenter::analyze() {
    int changedClusters = 0;
    std::queue<std::shared_ptr<file_entry>> queue;
    for(auto it = m_rootEntry->children.rbegin(); it != m_rootEntry->children.rend(); ++it) {
        queue.push(*it);
    }

    while(!queue.empty()) {
        auto actual = std::move(queue.front());
        queue.pop();
        std::printf("Zkoumam soubor: %s\n", getFullPath(actual).c_str());

        if (actual->me->file_type == Fat::FILE_TYPE_DIRECTORY) { // Jedná-li se o složku, přidám její obsah na konec zásobníku
            for(auto it = actual->children.rbegin(); it != actual->children.rend(); ++it) {
                queue.push(*it);
            }
        } else { // Nejedná-li se o složku, analyzuji soubor
            auto clusters = m_fat.getClusters(actual->me);
            auto badClusterIndex = needReplace(clusters);
            if (badClusterIndex == 0) {
                continue;
            }
            std::printf("Soubor %s potřebuje transfer\n", getFullPath(actual).c_str());
            changedClusters++;

            auto lastGoodClusterIndex = badClusterIndex - 1;
            auto goodCluster = clusters.at(lastGoodClusterIndex);
            for(unsigned int i = badClusterIndex; i < clusters.size(); i++) {
                const auto &clusterToReplace = clusters.at(i);
                auto newCluster = goodCluster + 1;
                while(m_fat.m_workingFatTable[newCluster] == Fat::FAT_DIRECTORY_CONTENT
                      || m_fat.m_workingFatTable[newCluster] == Fat::FAT_BAD_CLUSTER) {
                    goodCluster++;
                    newCluster = goodCluster + 1;
                }

                std::printf("Přesouvám cluster %d na novou pozici: %d\n", clusterToReplace, newCluster);
                swapFatRegistry(clusterToReplace, newCluster);
                goodCluster++;
            }

            if (changedClusters > 0) {
                return false;
            }
        }
    }

    return changedClusters == 0;
}

// Zjistí, zda-li je potřeba shluknout obsah souboru. Pokud ano, vrátí index prvního clusteru, který musí být upraven
const unsigned int Defragmenter::needReplace(const std::vector<unsigned int> &clusters) {
    if (clusters.empty()) {
        return 0;
    }

    auto index = clusters.front();
    for(unsigned int i = 0; i < clusters.size(); i++) {
        const auto &cluster = clusters.at(i);

        if (index != cluster) {
            if (m_fat.m_workingFatTable[index] == Fat::FAT_DIRECTORY_CONTENT) {
                while (m_fat.m_workingFatTable[index] == Fat::FAT_DIRECTORY_CONTENT) {
                    index++;
                    if (index >= m_fat.m_BootRecord->cluster_count) {
                        return 0;
                    }
                }
                if (index == cluster) {
                    index++;
                    continue;
                }
            }
            return i;
        }

        index++;
    }

    return 0;
}

// Prohodí dva záznamy ve fat tabulce
void Defragmenter::swapFatRegistry(const unsigned int lhs, const unsigned int rhs) {
    auto lhsContent = m_fat.m_workingFatTable[lhs];
    auto rhsContent = m_fat.m_workingFatTable[rhs];
    auto lhsParent = findParentClusterIndex(lhs);
    auto rhsParent = findParentClusterIndex(rhs);

    assert(lhsParent != Fat::FAT_DIRECTORY_CONTENT);
    if (rhsParent == Fat::FAT_DIRECTORY_CONTENT) { // Chci swapnout content s clusterem, co obsahuje složku
        std::shared_ptr<file_entry> parentFileEntry{nullptr};
        try {
            parentFileEntry = findParentFileEntry(rhs);
            auto offset = parentFileEntry->parent->me->first_cluster;
            auto folders = m_fat.loadDirectory(offset);

            for (auto &&item : folders) {
                if (item->first_cluster == rhs) {
                    item->first_cluster = lhs;
                    m_fat.m_workingFatTable[lhs] = rhsContent;
                    m_fat.m_workingFatTable[lhsParent] = rhs;
                    m_fat.m_workingFatTable[rhs] = lhsContent;

                    break;
                }
            }

            m_fat.saveClusterWithFiles(folders, offset);
        } catch (std::exception &ex) {
            swapClusters(lhs, rhs);

            auto tmp = lhsContent;
            m_fat.m_workingFatTable[lhs] = rhsContent;
            m_fat.m_workingFatTable[lhsParent] = rhs;
            m_fat.m_workingFatTable[rhs] = tmp;
        }


    } else {
        swapClusters(lhs, rhs);

        auto tmp = lhsContent;
        if (rhsContent != Fat::FAT_UNUSED) {
            m_fat.m_workingFatTable[rhsParent] = lhs;
        }
        m_fat.m_workingFatTable[lhs] = rhsContent;
        m_fat.m_workingFatTable[lhsParent] = rhs;
        m_fat.m_workingFatTable[rhs] = tmp;
    }
}

// Prohodí obsah dvou clusterů
void Defragmenter::swapClusters(const unsigned int lhs, const unsigned int rhs) {
    auto lhsContent = m_fat.getClusterContent(lhs);
    auto rhsContent = m_fat.getClusterContent(rhs);

    m_fat.writeClusterContent(lhs, rhsContent);
    m_fat.writeClusterContent(rhs, lhsContent);
}

// Najde záznam ve fat tabulce, který ukazuje na zadaného potomka. Pokud nenajde, tak vrátí, že se pravděpodobně
// jedná o složku, na kterou se musí použít těžší kalibr
const unsigned int Defragmenter::findParentClusterIndex(const unsigned int t_child) {
    if (m_fat.m_workingFatTable[t_child] == Fat::FAT_UNUSED) {
        return Fat::FAT_UNUSED;
    }

    for (unsigned int i = 0; i < m_fat.m_BootRecord->cluster_count; ++i) {
        if (m_fat.m_workingFatTable[i] == t_child) {
            return i;
        }
    }

    return Fat::FAT_DIRECTORY_CONTENT;
}

const std::shared_ptr<file_entry> Defragmenter::findParentFileEntry(const unsigned int t_child) {
    std::queue<std::shared_ptr<file_entry>> queue;
    for (auto &&item : m_rootEntry->children) {
        queue.push(item);
    }

    while(!queue.empty()) {
        auto actual = std::move(queue.front());
        queue.pop();

        for (auto &&item :actual->children) {
            if (item->me->first_cluster == t_child) {
                return item;
            }

            queue.push(item);
        }
    }

    throw std::runtime_error("Parent not found");
}