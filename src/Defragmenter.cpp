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

Defragmenter::Defragmenter(Fat &t_fat) : m_fat(t_fat){}

// Spustí defragmentaci fatky
void Defragmenter::runDefragmentation() {

}

// Nechá fatku vypsat stromovu strukturu, kde jsou vidět čísla clusterů
void Defragmenter::printTree() {
    m_fat.tree();
}
