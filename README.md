# CS6715-Program-Analysis

VM image: https://drive.google.com/file/d/1Skn4p6HOG9drHliDj_nt2ysEL9DNqeIQ/view?usp=sharing
Please download the latest version of VirtualBox and import the image.

## Install LLVM-14 from source
1. Copy the script:
```
cp buildllvm14.sh /root
cd /root
```

2. Build and install:
```
chmod +x buildllvm14.sh
./buildllvm14.sh
```

3. Add LLVM tools to PATH (persistent):
```
echo 'export PATH=/root/build/bin:$PATH' >> /root/.bashrc
```

4. Reload PATH:
```
source /root/.bashrc
```

5. Quick verification:
```
which clang opt
clang --version
opt --version
```
