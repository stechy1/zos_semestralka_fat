#!/bin/bash

FILES=`ls files`
fatFile='test.fat'

rm -rf build
mkdir build
cd build

cmake ../
make

for file in ${FILES}; do
    cp "../files/$file" "../build/$file"
done

cp "../fat_maker.sh" "./fat_maker.sh"
chmod u+x ./fat_maker.sh
./fat_maker.sh ${fatFile}
cp "../defragmenter.sh" "./defragmenter.sh"
chmod u+x ./defragmenter.sh
