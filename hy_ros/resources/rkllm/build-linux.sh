set -e
rm -rf build
mkdir build && cd build

        
cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/aarch64-linux-gnu-g++  \
        -DCMAKE_C_COMPILER=/usr/bin/aarch64-linux-gnu-gcc \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=aarch64 \

make -j8
make install