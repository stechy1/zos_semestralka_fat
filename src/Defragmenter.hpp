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

#ifndef ZOS_SEMESTRALKA_FAT_DEFRAGMENTATOR_H
#define ZOS_SEMESTRALKA_FAT_DEFRAGMENTATOR_H


#include "Fat.hpp"

struct file_entry {
    std::shared_ptr<file_entry> parent;
    std::shared_ptr<root_directory> me;
    std::vector<std::shared_ptr<file_entry>> children;
};

class Defragmenter {

public:
    Defragmenter(Fat &t_fat);

    Defragmenter(const Defragmenter &other) = delete;

    Defragmenter &operator=(const Defragmenter &other) = delete;

    Defragmenter(Defragmenter &&other) = delete;

    Defragmenter &operator=(Defragmenter &&other) = delete;

    virtual ~Defragmenter();

    void runDefragmentation();

    void printTree();

private:
    Fat &m_fat;
    unsigned int *m_translationTable;
    std::shared_ptr<file_entry> m_rootEntry;

    void loadFullTree();

    void loadSubTree(std::shared_ptr<file_entry> t_parent);

    void printSubTree(std::shared_ptr<file_entry> t_parent, unsigned int t_depth);
};


#endif //ZOS_SEMESTRALKA_FAT_DEFRAGMENTATOR_H
