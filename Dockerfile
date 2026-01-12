FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install basic build tools and dependencies
# graphviz is included for dot2png.sh
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    lsb-release \
    software-properties-common \
    gnupg \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM 14 and Clang 14
RUN apt-get update && apt-get install -y \
    llvm-14 \
    llvm-14-dev \
    llvm-14-tools \
    clang-14 \
    libclang-14-dev \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables so CMake can find LLVM 14 automatically
ENV LLVM_DIR=/usr/lib/llvm-14/lib/cmake/llvm
ENV PATH="/usr/lib/llvm-14/bin:${PATH}"

WORKDIR /usr/src/app

CMD ["/bin/bash"]