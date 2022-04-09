#!/usr/bin/env bash

if [ ! -d ./cmake-build-release ]; then
    mkdir ./cmake-build-release
fi

cd cmake-build-release || exit
rm -rf *
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ..

cmake --build . --target clean -- -j 8
cmake --build . --target install -- -j 8
cmake --build . --target docks-server -- -j 8
cmake --build . --target docks-client -- -j 8
if [ -f ./docks/docks-server ]; then
    if [ ! -d ../release ]; then
        mkdir ../release
    fi
    cp -v ./docks/docks-server ../release/docks-server
    cp -v ./docks/docks-client ../release/docks-client
fi