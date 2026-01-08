#  !bash
Version=14.0.6

# 1. extract source code
if [ ! -d "llvm-$Version.src" ]; then
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-$Version/llvm-$Version.src.tar.xz
    tar -xvf llvm-$Version.src.tar.xz

    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-$Version/clang-$Version.src.tar.xz
    tar -xvf clang-$Version.src.tar.xz
    mv clang-$Version.src llvm-$Version.src/tools/

    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-$Version/compiler-rt-$Version.src.tar.xz
    tar -xvf compiler-rt-$Version.src.tar.xz
    mv compiler-rt-$Version.src llvm-$Version.src/projects/

    apt install -y autoconf bison flex m4 libgmp-dev libmpfr-dev libssl-dev binutils-dev
fi

# 2. build llvm
if [ ! -d "build" ]; then mkdir build; fi
cd build
cmake -DCMAKE_BUILD_TYPE:String=Release -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_ENABLE_TERMINFO=OFF ../llvm-$Version.src
make -j16
cd -