#!/bin/bash

buildType=$1
if [ "$buildType" == "" ]; then
	buildType="Release"
fi

if [ ! -d "build" ]; then
	mkdir build
fi

cd build
cmake -DCMAKE_BUILD_TYPE=$buildType ../myAnalysis || exit 1
make || exit 1
cd -
