#!/bin/bash

buildType=$1
if [ "$buildType" == "" ]; then
	buildType="Release"
fi

if [ ! -d "build" ]; then
	mkdir build
fi

cd build
cmake -DCMAKE_BUILD_TYPE=$buildType ../myAnalysis
make
cd -

# build benchmarks
cd benchmark && ./build.sh && cd -