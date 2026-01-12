# CS6715-Program-Analysis

VM image: https://drive.google.com/file/d/1Skn4p6HOG9drHliDj_nt2ysEL9DNqeIQ/view?usp=sharing
Please download the latest version of VirtualBox and import the image.

## Install LLVM-14 from source (for ones who need to setup environment in container)
```
# 1) copy the script and run it (inside container)
cp buildllvm14.sh /root
cd /root
chmod +x buildllvm14.sh
./buildllvm14.sh

# 2) add LLVM tools to PATH (persistent for root)
echo 'export PATH=/root/build/bin:$PATH' >> /root/.bashrc
source /root/.bashrc

# 3) verify
which clang
clang --version
```

