#!/bin/sh
# executed by `post-install.txt` as `chroot /rootfs find /tmp/extras -type f -exec sh {} \;`
# executed by user as `sudo ./netinst.sh`

echo ""
echo "=== Adding libPCA9685 ==="

# activate i2c
echo "dtparam=i2c_arm=on" >> /boot/config.txt
echo "dtparam=i2c_arm_baudrate=1000000" >> /boot/config.txt
echo "options i2c_bcm2708 combined=1" >> /etc/modprobe.d/i2c_repeated_start.conf
echo "i2c-dev" >> /etc/modules

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

# download libPCA9685
echo ""
echo "= Cloning libPCA9685"
/usr/bin/git clone git://github.com/edlins/libPCA9685 /usr/local/src/libPCA9685

# build and install libPCA9685
cd /usr/local/src/libPCA9685
echo ""
echo "= Building libPCA9685"
mkdir build && cd build
cmake ..
make
echo ""
echo "= Installing libPCA9685"
make install
