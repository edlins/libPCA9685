#!/bin/sh
# executed by `post-install.txt` as `chroot /rootfs find /tmp/extras -type f -exec sh {} \;`
# executed by user as `sudo ./netinst.sh`

# olaclient depends on libPCA9685 so that should be installed first
# for simplicity, this script will not duplicate what was already done

echo ""
echo "=== Adding olaclient ==="

# install make
echo ""
echo "= Adding make"
/usr/bin/apt-get -y --no-install-recommends install make

# install cmake
echo ""
echo "= Adding cmake"
/usr/bin/apt-get -y --no-install-recommends install cmake

# install g++
echo ""
echo "= Adding g++"
/usr/bin/apt-get -y --no-install-recommends install g++

# install libola-dev
echo ""
echo "= Adding libola-dev"
/usr/bin/apt-get -y --no-install-recommends install libola-dev

# build and install olaclient
cd /usr/local/src/libPCA9685
echo ""
echo "= Building olaclient"
# `mkdir build && cd build` fails if /usr/local/src/libPCA9685/build already exists
#mkdir build && cd build
mkdir build
cd build
cmake ..
make olaclient
echo ""
echo "= Installing olaclient"
cd examples/olaclient
make install
