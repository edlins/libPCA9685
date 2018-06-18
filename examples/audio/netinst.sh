#!/bin/sh
# executed by `post-install.txt` as `chroot /rootfs find /tmp/extras -type f -exec sh {} \;`
# executed by user as `sudo ./netinst.sh`
# plug in your usb sound card before running
# it should appear as something like:
#   $ aplay -L
#   [...]
#   plughw:CARD=Device,DEV=0
#   C-Media USB Audio Device, USB Audio
#   Hardware device with all software conversions


echo ""
echo "=== Adding audio ==="

echo ""
echo "= Turning audio on"
echo "dtparam=audio=on" >> /boot/config.txt

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

# install libasound2-dev
echo ""
echo "= Adding libasound2-dev"
/usr/bin/apt-get -y --no-install-recommends install libasound2-dev

# install alsa-utils
echo ""
echo "= Adding alsa-utils"
/usr/bin/apt-get -y --no-install-recommends install alsa-utils

# install libfftw3-dev
echo ""
echo "= Adding libfftw3-dev"
/usr/bin/apt-get -y --no-install-recommends install libfftw3-dev

# build and install audio
cd /usr/local/src/libPCA9685/examples/audio
echo ""
echo "= Building audio"
mkdir build
cd build
cmake ..
make
echo ""
echo "= Installing audio"
make install
