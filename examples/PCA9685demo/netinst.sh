#!/bin/sh
# executed by `post-install.txt` as `chroot /rootfs find /tmp/extras -type f -exec sh {} \;`
# executed by user as `sudo ./netinst.sh`

echo ""
echo "=== Adding PCA9685demo ==="

# install libncurses5-dev
echo ""
echo "= Adding libncurses5-dev"
/usr/bin/apt-get -y --no-install-recommends install libncurses5-dev

# build and install PCA9685demo
cd /usr/local/src/libPCA9685
echo ""
echo "= Building PCA9685demo"
mkdir build
cd build
cmake ..
make PCA9685demo
echo ""
echo "= Installing PCA9685demo"
cd examples/PCA9685demo
make install
