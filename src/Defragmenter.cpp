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

#include "Defragmenter.hpp"
#include "ThreadPool.hpp"

Defragmenter::Defragmenter(Fat &t_fat) : m_fat(t_fat) {
    m_translationTable = new unsigned int[m_fat.m_BootRecord->cluster_count];
    for (int i = 0; i < m_fat.m_BootRecord->cluster_count; ++i) {
        m_translationTable[i] = Fat::FAT_UNUSED;
    }

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
}

// Nechá fatku vypsat stromovu strukturu, kde jsou vidět čísla clusterů
void Defragmenter::printTree() {
    std::printf("+%s\n", m_rootEntry->me->file_name);

    for (auto &item : m_rootEntry->children) {
        printSubTree(item, Fat::SPACE_SIZE);
    }
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
        for (auto &itemContent : content) {
            auto rootEntry = std::make_shared<file_entry>();
            rootEntry->me = itemContent;
            rootEntry->parent = child;
            child->children.push_back(rootEntry);
        }
        DefaultThreadPool::submitJob([&](std::shared_ptr<file_entry> fileEntry) {
            std::printf("%s\n", fileEntry->me->file_name);
            loadSubTree(fileEntry);
        }, child);
    }

    for (auto &&item : vector) {
        item.get();
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
